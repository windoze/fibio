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
    , runner_(([this](caller_t &c){ runner_wrapper(c); }))
    , caller_(0)
    {}
    
    fiber_object::~fiber_object() {
        //printf("Destroy fiber 0x%lx \n", reinterpret_cast<unsigned long>(this));
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
        c([](){});
        
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
            // Is there any way to do stack trace?
            assert(false);
        }
        // Fiber function exits, set state to STOPPED
        c([this](){
            state_=STOPPED;
        });
        caller_=0;
    }
    
    void fiber_object::schedule() {
        std::lock_guard<std::mutex> lock(fiber_mutex_);
        fiber_ptr_t pthis(shared_from_this());
        fiber_strand_.post([pthis](){
            pthis->state_=READY;
            pthis->one_step();
        });
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
        //assert(state_!=BLOCKED);
        if (state_==READY) {
            state_=RUNNING;
        }
        while (state_==RUNNING) {
            if (runner_) {
                after_step_handler_t h;
                {
                    tls_guard guard(this);
                    h=runner_().get();
                }
                // Change state to READY, schedule for next round unless after_step_handler explicitly change it back
                state_=READY;
                h();
            } else {
                // ERROR: Should not happen
                state_=STOPPED;
            }
            // Keep running if the state remains RUNNING
        }
        state_t s= state_;
        if (s==READY) {
            // Post this fiber to the scheduler
            schedule();
        } else if (s==BLOCKED) {
            // Do nothing
            // Must make sure this fiber will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            std::lock_guard<std::mutex> lock(fiber_mutex_);
            // Fiber ended, clean up join queue
            for (std::function<void()> f: cleanup_queue_) {
                f();
            }
            cleanup_queue_.clear();
            // Clean up FSS
            for (auto &v: fss_) {
                if (v.second.first && v.second.second) {
                    (*(v.second.first))(v.second.second);
                }
            }
            // Post exit message to scheduler
            scheduler_ptr_t s=sched_;
            fiber_ptr_t this_fiber(shared_from_this());
            fiber_strand_.post([s, this_fiber](){
                s->on_fiber_exit(this_fiber);
            });
            //printf("fiber 0x%lx existed\n", reinterpret_cast<unsigned long>(this));
        }
    }
    
    void fiber_object::throw_on_error() {
        if (last_error_) {
            // FIXME: ASIO uses more than one error categories
            //last_error_=make_error_code(static_cast<boost::system::errc>(last_error_.value()));
            throw boost::system::system_error(last_error_);
        }
    }
    
    // Switch out of fiber context
    void fiber_object::pause() {
        CHECK_CALLER(this);
        fiber_ptr_t pthis(shared_from_this());
        (*caller_)([pthis](){
            pthis->state_=BLOCKED;
        });
    }
    
    // Following functions can only be called inside coroutine
    void fiber_object::yield() {
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis](){
                pthis->state_=READY;
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
    }
    
    void fiber_object::join(fiber_ptr_t f) {
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis, f](){
                std::lock_guard<std::mutex> lock(f->fiber_mutex_);
                if (f==pthis) {
                    // The fiber is joining itself
                    pthis->last_error_=make_error_code(boost::system::errc::resource_deadlock_would_occur) ;
                } else if (f->state_==STOPPED) {
                    // f is already stopped
                    pthis->state_=RUNNING;
                } else {
                    // std::cout << "fiber(pthis) blocked" << std::endl;
                    pthis->state_=BLOCKED;
                    f->cleanup_queue_.push_back([pthis](){
                        // std::cout << "fiber(pthis) resumed" << std::endl;
                        pthis->state_=READY;
                        pthis->one_step();
                    });
                }
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
    }
    
    void fiber_object::join_and_rethrow(fiber_ptr_t f) {
        CHECK_CALLER(this);
        std::nested_exception e;
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis, f, &e](){
                std::lock_guard<std::mutex> lock(f->fiber_mutex_);
                if (f==pthis) {
                    // The fiber is joining itself
                    pthis->last_error_=make_error_code(boost::system::errc::resource_deadlock_would_occur) ;
                } else if (f->state_==STOPPED) {
                    // f is already stopped
                    if (f->uncaught_exception_.nested_ptr()) {
                        // Propagate uncaught exception in f to this fiber
                        e=f->uncaught_exception_;
                        // Clean uncaught exception in f
                        f->uncaught_exception_=std::nested_exception();
                    }
                    pthis->state_=RUNNING;
                } else {
                    // std::cout << "fiber(pthis) blocked" << std::endl;
                    pthis->state_=BLOCKED;
                    f->cleanup_queue_.push_back([pthis, f, &e](){
                        if (f->uncaught_exception_.nested_ptr()) {
                            // Propagate uncaught exception in f to this fiber
                            e=f->uncaught_exception_;
                            // Clean uncaught exception in f
                            f->uncaught_exception_=std::nested_exception();
                        }
                        // std::cout << "fiber(pthis) resumed" << std::endl;
                        pthis->state_=READY;
                        pthis->one_step();
                    });
                }
            });
        } else {
            // TODO: Error
        }
        
        // throw propagated exception
        if (e.nested_ptr()) {
            e.rethrow_nested();
        }
        
        throw_on_error();
    }
    
    void fiber_object::sleep_usec(uint64_t usec) {
        // Shortcut
        if (usec==0) {
            return;
        }
        CHECK_CALLER(this);
        if (caller_) {
            fiber_ptr_t pthis(shared_from_this());
            (*caller_)([pthis, usec](){
                pthis->state_=BLOCKED;
                timer_ptr_t sleep_timer(std::make_shared<timer_t>(pthis->io_service_));
                sleep_timer->expires_from_now(std::chrono::microseconds(usec));
                sleep_timer->async_wait(pthis->fiber_strand_.wrap([pthis, sleep_timer](boost::system::error_code ec){
                    if (ec) {
                        pthis->last_error_=ec;
                    }
                    pthis->state_=RUNNING;
                    pthis->one_step();
                }));
            });
        } else {
            // TODO: Error
        }
        
        throw_on_error();
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
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
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
            // this_fiber->schedule();
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
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
            // this_fiber->schedule();
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
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
            // this_fiber->schedule();
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
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
            // this_fiber->schedule();
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
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
            // this_fiber->schedule();
            this_fiber->state_=fibers::detail::fiber_object::RUNNING;
            this_fiber->one_step();
        }
    }
    
    void fiber_async_handler::run_in_scheduler_context(std::function<void()> f) {
        CHECK_CALLER(this_fiber);
        (*(this_fiber->caller_))([this, &f](){
            this_fiber->state_=fibers::detail::fiber_object::BLOCKED;
            f();
        });
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
    
    void fiber::set_name(const std::string &s) {
        m_->set_name(s);
    }
    
    std::string fiber::get_name() {
        return m_->get_name();
    }
    
    bool fiber::joinable() const {
        // Return true iff this is a fiber and not the current calling fiber
        return (m_ && detail::fiber_object::current_fiber_!=m_.get());
    }
    
    fiber::id fiber::get_id() const {
        return reinterpret_cast<fiber::id>(m_.get());
    }
    
    void fiber::join(bool propagate_exception) {
        if (!m_) {
            throw make_error_code(boost::system::errc::no_such_process);
        }
        if (m_.get()==detail::fiber_object::current_fiber_) {
            throw make_error_code(boost::system::errc::resource_deadlock_would_occur);
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
        detail::fiber_ptr_t this_fiber=m_;
        m_->fiber_strand_.post([this_fiber](){
            this_fiber->detach();
        });
        m_.reset();
    }
    
    void fiber::swap(fiber &other) noexcept(true) {
        std::swap(m_, other.m_);
    }
    
    unsigned fiber::hardware_concurrency() {
        return std::thread::hardware_concurrency();
    }
    
    namespace this_fiber {
        void yield() {
            if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                ::fibio::fibers::detail::fiber_object::current_fiber_->yield();
            } else {
                throw boost::system::system_error(make_error_code(boost::system::errc::no_such_process));
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
                    throw boost::system::system_error(make_error_code(boost::system::errc::no_such_process));
                }
            }
            
            boost::asio::io_service &get_io_service() {
                if (::fibio::fibers::detail::fiber_object::current_fiber_) {
                    return ::fibio::fibers::detail::fiber_object::current_fiber_->io_service_;
                }
                throw boost::system::system_error(make_error_code(boost::system::errc::no_such_process));
            }
        }   // End of namespace detail
    }   // End of namespace this_fiber
}}  // End of namespace fibio::fibers

