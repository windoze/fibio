//
//  fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <memory>
#include <algorithm>
#include <system_error>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/fss.hpp>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

#include "fiber_object.hpp"
#include "scheduler_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    __thread fiber_object *fiber_object::current_fiber_=0;
    
    fiber_object::fiber_object(scheduler_ptr_t sched, entry_t &&entry)
    : sched_(sched)
    , io_service_(sched->io_service_)
    , fiber_strand_(io_service_)
    , state_(READY)
    , entry_(std::move(entry))
    , runner_(std::bind(&fiber_object::runner_wrapper, this, std::placeholders::_1) )
    , caller_(0)
    {}
    
    fiber_object::~fiber_object() {
        //printf("Destroy fiber 0x%lx \n", reinterpret_cast<unsigned long>(this));
        if (state_!=STOPPED) {
            // std::thread will call std::terminate if deleting a unstopped thread
            std::terminate();
        }
    }
    
    void fiber_object::set_name(const std::string &s) {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        name_=s;
    }
    
    std::string fiber_object::get_name() {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        return name_;
    }
    
    void fiber_object::runner_wrapper(caller_t &c) {
        // Need this to complete constructor without running entry_
        c(READY);
        
        // Now we're out of constructor
        caller_=&c;
        try {
            entry_();
        } catch(const boost::coroutines::detail::forced_unwind&) {
            // Boost.Coroutine requirement
            throw;
        } catch(std::exception &e) {
            // This exception can be propagated to joiner
            // HACK: Why there is no way to just create a nested_exception?
            try {
                std::throw_with_nested(e);
            } catch (std::nested_exception &ne) {
                std::lock_guard<std::mutex> guard(fiber_mutex_);
                uncaught_exception_=ne;
            }
        } catch(...) {
            // TODO: Uncaught unknown exception
            // This is standard action for threads, is this appropriate for fibers?
            // Is there any way to do stack trace?
            std::terminate();
        }
        // Fiber function exits, set state to STOPPED
        c(STOPPED);
        caller_=0;
    }
    
    void fiber_object::schedule() {
        fiber_strand_.post(std::bind(&fiber_object::activate, shared_from_this()));
    }
    
    void fiber_object::detach() {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        if (state_!=STOPPED) {
            // Hold a reference to this, make sure detached fiber live till ends
            this_ref_=shared_from_this();
        }
    }
    
    void fiber_object::one_step() {
        struct tls_guard {
            tls_guard(fiber_object *pthis) {
                fiber_object::current_fiber_=pthis;
            }
            
            ~tls_guard() {
                fiber_object::current_fiber_=0;
            }
        };
        if (state_==READY) {
            state_=RUNNING;
        }
        // Keep running if necessary
        while (state_==RUNNING) {
            tls_guard guard(this);
            state_=runner_().get();
        }
        state_t s= state_;
        if (s==READY) {
            // Post this fiber to the scheduler
            schedule();
        } else if (s==BLOCKED) {
            // Do nothing
            // Must make sure this fiber will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            cleanup_queue_t temp;
            {
                // Move joining queue content out
                std::lock_guard<std::mutex> lock(fiber_mutex_);
                temp.swap(cleanup_queue_);
            }
            // Fiber ended, clean up joining queue
            for (std::function<void()> f: temp) {
                f();
            }
            // Clean up FSS
            for (auto &v: fss_) {
                if (v.second.first && v.second.second) {
                    (*(v.second.first))(v.second.second);
                }
            }
            // Post exit message to scheduler
            fiber_strand_.post(std::bind(&scheduler_object::on_fiber_exit, sched_, shared_from_this()));
        }
    }
    
    void fiber_object::throw_on_error() {
        if (last_error_) {
            throw boost::system::system_error(last_error_);
        }
    }
    
    boost::asio::strand &fiber_object::get_fiber_strand() {
        return fiber_strand_;
    }
    
    // Switch out of fiber context
    void fiber_object::pause() {
        set_state(BLOCKED);
    }
    
    void fiber_object::activate() {
        state_=READY;
        one_step();
    }
    
    // Following functions can only be called inside coroutine
    void fiber_object::yield() {
        set_state(READY);
    }

    void fiber_object::join(fiber_ptr_t f) {
        CHECK_CALLER(this);
        {
            std::lock_guard<std::mutex> lock(f->fiber_mutex_);
            if (this==f.get()) {
                // The fiber is joining itself
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
            } else if (f->state_==STOPPED) {
                // f is already stopped, do nothing
                return;
            } else {
                f->cleanup_queue_.push_back(std::bind(&fiber_object::activate, shared_from_this()));
            }
        }
        pause();
    }
    
    void propagate_exception(fiber_ptr_t f) {
        std::nested_exception e;
        if (f->uncaught_exception_.nested_ptr()) {
            // Propagate uncaught exception in f to this fiber
            e=f->uncaught_exception_;
            // Clean uncaught exception in f
            f->uncaught_exception_=std::nested_exception();
        }
        // throw propagated exception
        if (e.nested_ptr()) {
            e.rethrow_nested();
        }
    }
    
    void fiber_object::join_and_rethrow(fiber_ptr_t f) {
        CHECK_CALLER(this);
        {
            std::lock_guard<std::mutex> lock(f->fiber_mutex_);
            if (this==f.get()) {
                // The fiber is joining itself
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
            } else if (f->state_==STOPPED) {
                // f is already stopped
                propagate_exception(f);
                return;
            } else {
                // std::cout << "fiber(pthis) blocked" << std::endl;
                f->cleanup_queue_.push_back(std::bind(&fiber_object::activate, shared_from_this()));
            }
        }
        pause();

        // Joining completed, propagate exception from joinee
        std::lock_guard<std::mutex> lock(f->fiber_mutex_);
        propagate_exception(f);
    }

    void fiber_object::sleep_usec(uint64_t usec) {
        // Shortcut
        if (usec==0) {
            return;
        }
        CHECK_CALLER(this);
        timer_t sleep_timer(io_service_);
        sleep_timer.expires_from_now(std::chrono::microseconds(usec));
        sleep_timer.async_wait(fiber_strand_.wrap(std::bind(&fiber_object::activate, shared_from_this())));

        pause();
    }
    
    void fiber_object::add_cleanup_function(std::function<void()> &&f) {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        cleanup_queue_.push_back(std::move(f));
    }
    
    void set_fss_data(void const* key,std::shared_ptr<fss_cleanup_function> func,void* fss_data,bool cleanup_existing) {
        if (fiber_object::current_fiber_) {
            if (!func && !fss_data) {
                // Remove fss if both func and data are NULL
                fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
                if (i!=fiber_object::current_fiber_->fss_.end() ) {
                    // Clean up existing if it has a cleanup func
                    if(i->second.first)
                        (*(i->second.first.get()))(i->second.second);
                    fiber_object::current_fiber_->fss_.erase(i);
                }
            } else {
                // Clean existing if needed
                if (cleanup_existing) {
                    fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
                    if (i!=fiber_object::current_fiber_->fss_.end() && (i->second.first)) {
                        // Clean up existing if it has a cleanup func
                        (*(i->second.first.get()))(i->second.second);
                    }
                }
                // Insert/update the key
                fiber_object::current_fiber_->fss_[key]={func, fss_data};
            }
        }
    }
    
    void* get_fss_data(void const* key) {
        if (fiber_object::current_fiber_) {
            fss_map_t::iterator i=fiber_object::current_fiber_->fss_.find(key);
            if (i!=fiber_object::current_fiber_->fss_.end()) {
                return i->second.second;
            } else {
                // Create if not exist
                //fiber_object::current_fiber_->fss_.insert({key, {std::shared_ptr<fss_cleanup_function>(), 0}});
            }
        }
        return 0;
    }
    
    fiber_base::ptr_t get_current_fiber_ptr() {
        if(!fiber_object::current_fiber_) {
            // Not a fiber
            throw fiber_exception(boost::system::errc::no_such_process);
        }
        return std::static_pointer_cast<fiber_base>(fiber_object::current_fiber_->shared_from_this());
    }
    
    fiber_async_handler::fiber_async_handler()
    : this_fiber(fibers::detail::fiber_object::current_fiber_->shared_from_this())
    , sleep_timer(this_fiber->io_service_)
    {}
    
    void fiber_async_handler::start_timer_with_cancelation(uint64_t timeout, std::function<void()> &&c) {
        timeout_=timeout;
        cancelation_=std::move(c);
        if(timeout_>0) {
            sleep_timer.expires_from_now(std::chrono::microseconds(timeout_));
            sleep_timer.async_wait(this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec){
                on_timeout(ec);
            }));
        }
    }
    
    std::function<void(boost::system::error_code ec, size_t sz)> fiber_async_handler::get_io_handler()
    { return this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec, size_t sz){ on_io_complete(ec, sz); }); }
    
    std::function<void(boost::system::error_code ec)> fiber_async_handler::get_async_op_handler()
    { return this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec){ on_async_op_complete(ec); }); }
    
    std::function<void(boost::system::error_code, boost::asio::ip::tcp::resolver::iterator)> fiber_async_handler::get_resolve_handler(boost::asio::ip::tcp *)
    {
        return this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator it){
            on_tcp_resolve_complete(ec, it);
        });
    }
    
    std::function<void(boost::system::error_code, boost::asio::ip::udp::resolver::iterator)> fiber_async_handler::get_resolve_handler(boost::asio::ip::udp *)
    {
        return this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec, boost::asio::ip::udp::resolver::iterator it){
            on_udp_resolve_complete(ec, it);
        });
    }
    
    std::function<void(boost::system::error_code, boost::asio::ip::icmp::resolver::iterator)> fiber_async_handler::get_resolve_handler(boost::asio::ip::icmp *)
    {
        return this_fiber->fiber_strand_.wrap([this](boost::system::error_code ec, boost::asio::ip::icmp::resolver::iterator it){
            on_icmp_resolve_complete(ec, it);
        });
    }
    

    void fiber_async_handler::on_timeout(boost::system::error_code ec) {
        timer_triggered=true;
        if(async_op_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        } else {
            if(cancelation_) cancelation_();
        }
    }
    
    void fiber_async_handler::on_async_op_complete(boost::system::error_code ec) {
        async_op_triggered=true;
        // Operation completed, cancel timer
        if(timeout_>0 && !timer_triggered) sleep_timer.cancel();
        if(ec==boost::asio::error::operation_aborted)
            ec=boost::asio::error::timed_out;
        this_fiber->last_error_=ec;
        if(timeout_==0 || timer_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        }
    }
    
    void fiber_async_handler::on_io_complete(boost::system::error_code ec, size_t sz) {
        async_op_triggered=true;
        // Operation completed, cancel timer
        sleep_timer.cancel();
        if(ec==boost::asio::error::operation_aborted)
            ec=boost::asio::error::timed_out;
        this_fiber->last_error_=ec;
        io_ret_=sz;
        if(timeout_==0 || timer_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        }
    }
    
    void fiber_async_handler::on_tcp_resolve_complete(boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator iterator) {
        async_op_triggered=true;
        // Operation completed, cancel timer
        sleep_timer.cancel();
        if(ec==boost::asio::error::operation_aborted)
            ec=boost::asio::error::timed_out;
        this_fiber->last_error_=ec;
        tcp_resolve_ret_=iterator;
        if(timeout_==0 || timer_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        }
    }
    
    void fiber_async_handler::on_udp_resolve_complete(boost::system::error_code ec, boost::asio::ip::udp::resolver::iterator iterator) {
        async_op_triggered=true;
        // Operation completed, cancel timer
        sleep_timer.cancel();
        if(ec==boost::asio::error::operation_aborted)
            ec=boost::asio::error::timed_out;
        this_fiber->last_error_=ec;
        udp_resolve_ret_=iterator;
        if(timeout_==0 || timer_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        }
    }
    
    void fiber_async_handler::on_icmp_resolve_complete(boost::system::error_code ec, boost::asio::ip::icmp::resolver::iterator iterator) {
        async_op_triggered=true;
        // Operation completed, cancel timer
        sleep_timer.cancel();
        if(ec==boost::asio::error::operation_aborted)
            ec=boost::asio::error::timed_out;
        this_fiber->last_error_=ec;
        icmp_resolve_ret_=iterator;
        if(timeout_==0 || timer_triggered) {
            // Both callback are called, resume fiber
            this_fiber->activate();
        }
    }
    
    void fiber_async_handler::pause_current_fiber() {
        this_fiber->pause();
    }
    
    void fiber_async_handler::throw_or_return(bool throw_error, boost::system::error_code &ec) {
        if (throw_error) {
            this_fiber->throw_on_error();
        } else {
            ec=this_fiber->last_error_;
            this_fiber->last_error_.clear();
        }
    }
}}}   // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
#ifdef NO_VARIADIC_TEMPLATE
    fiber::fiber(std::function<void()> &&f)
    : m_(scheduler::get_instance().m_->make_fiber(std::move(f)))
    {}
    
    fiber::fiber(scheduler &s, std::function<void()> &&f)
    : m_(s.m_->make_fiber(std::move(f)))
    {}
#else
    void fiber::start(std::function<void ()> &&f) {
        m_=scheduler::get_instance().m_->make_fiber(std::move(f));
    }
#endif
    
    fiber::fiber(fiber &&other) noexcept
    : m_(std::move(other.m_))
    {}
    
    fiber::fiber(fibio::fibers::scheduler &s, std::function<void ()> &&f)
    : m_(s.m_->make_fiber(std::move(f)))
    {}
    
    fiber& fiber::operator=(fiber &&other) noexcept {
        if (joinable()) {
            // This fiber is still active, std::thread will call std::terminate in the case
            std::terminate();
        }
        m_=std::move(other.m_);
        return *this;
    }
    
    void fiber::set_name(const std::string &s) {
        m_->set_name(s);
    }
    
    std::string fiber::get_name() {
        return m_->get_name();
    }
    
    bool fiber::joinable() const noexcept {
        // Return true iff this is a fiber and not the current calling fiber
        return (m_ && detail::fiber_object::current_fiber_!=m_.get());
    }
    
    fiber::id fiber::get_id() const noexcept {
        return reinterpret_cast<fiber::id>(m_.get());
    }
    
    void fiber::join(bool propagate_exception) {
        if (!m_) {
            throw fiber_exception(boost::system::errc::no_such_process);
        }
        if (m_.get()==detail::fiber_object::current_fiber_) {
            throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
        }
        if (!joinable()) {
            throw invalid_argument();
        }
        if (detail::fiber_object::current_fiber_) {
            if (propagate_exception) {
                detail::fiber_object::current_fiber_->join_and_rethrow(m_);
            } else {
                detail::fiber_object::current_fiber_->join(m_);
            }
        }
    }
    
    void fiber::detach() {
        if (!joinable()) {
            throw fiber_exception(boost::system::errc::no_such_process);
        }
        detail::fiber_ptr_t this_fiber=m_;
        m_->fiber_strand_.post(std::bind(&detail::fiber_object::detach, m_));
        m_.reset();
    }
    
    void fiber::swap(fiber &other) noexcept(true) {
        std::swap(m_, other.m_);
    }
    
    unsigned fiber::hardware_concurrency() noexcept {
        return std::thread::hardware_concurrency();
    }
    
    namespace this_fiber {
        void yield() {
            if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                ::fibio::fibers::detail::fiber_object::current_fiber_->yield();
            } else {
                throw fiber_exception(boost::system::errc::no_such_process);
            }
        }
        
        fiber::id get_id() {
            return reinterpret_cast<fiber::id>(::fibio::fibers::detail::fiber_object::current_fiber_);
        }
        
        bool is_a_fiber() noexcept(true) {
            return ::fibio::fibers::detail::fiber_object::current_fiber_;
        }
        
        namespace detail {
            void sleep_usec(uint64_t usec) {
                if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                    ::fibio::fibers::detail::fiber_object::current_fiber_->sleep_usec(usec);
                } else {
                    throw fiber_exception(boost::system::errc::no_such_process);
                }
            }
            
            boost::asio::io_service &get_io_service() {
                if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                    return ::fibio::fibers::detail::fiber_object::current_fiber_->io_service_;
                }
                throw fiber_exception(boost::system::errc::no_such_process);
            }
        }   // End of namespace detail
    }   // End of namespace this_fiber
}}  // End of namespace fibio::fibers

