//
//  fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <memory>
#include <algorithm>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/fss.hpp>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

#include "fiber_object.hpp"
#include "scheduler_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    __thread fiber_object *fiber_object::current_fiber_=0;
    
    fiber_object::fiber_object(scheduler_ptr_t sched, entry_t entry)
    : sched_(sched)
    , fiber_strand_(std::make_shared<boost::asio::strand>(sched_->io_service_))
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&fiber_object::runner_wrapper, this, std::placeholders::_1) )
    , caller_(0)
    {}
    
    fiber_object::fiber_object(scheduler_ptr_t sched, strand_ptr_t strand, entry_t entry)
    : sched_(sched)
    , fiber_strand_(strand)
    , state_(READY)
    , entry_(entry)
    , runner_(std::bind(&fiber_object::runner_wrapper, this, std::placeholders::_1) )
    , caller_(0)
    {}
    
    fiber_object::~fiber_object() {
        if (state_!=STOPPED) {
            // std::thread will call std::terminate if deleting a unstopped thread
            std::terminate();
        }
        if (uncaught_exception_ != std::exception_ptr()) {
            // There is an uncaught exception not propagated to joiner
            std::terminate();
        }
    }
    
    void fiber_object::set_name(const std::string &s) {
        std::lock_guard<std::mutex> lock(mtx_);
        name_=s;
    }
    
    std::string fiber_object::get_name() {
        std::lock_guard<std::mutex> lock(mtx_);
        return name_;
    }
    
    void fiber_object::runner_wrapper(caller_t &c) {
        // Need this to complete constructor without running entry_
        c(READY);
        
        // Now we're out of constructor
        caller_=&c;
        try {
            entry_->run();
        } catch(const boost::coroutines::detail::forced_unwind&) {
            // Boost.Coroutine requirement
            throw;
        } catch(...) {
            uncaught_exception_=std::current_exception();
        }
        // Fiber function exits, set state to STOPPED
        c(STOPPED);
        caller_=0;
    }
    
    void fiber_object::detach() {
        std::lock_guard<std::mutex> lock(mtx_);
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
            resume();
        } else if (s==BLOCKED) {
            // Must make sure this fiber will be posted elsewhere later, otherwise it will hold forever
        } else if (s==STOPPED) {
            cleanup_queue_t temp;
            {
                // Move joining queue content out
                std::lock_guard<std::mutex> lock(mtx_);
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
            get_fiber_strand().post(std::bind(&scheduler_object::on_fiber_exit, sched_, shared_from_this()));
        }
    }
    
    boost::asio::strand &fiber_object::get_fiber_strand() {
        return *fiber_strand_;
    }
    
    // Switch out of fiber context
    void fiber_object::pause() {
        // Pre-condition
        // Can only pause current running fiber
        BOOST_ASSERT(current_fiber_==this);
        
        set_state(BLOCKED);
    }
    
    inline void activate_fiber(fiber_ptr_t this_fiber) {
        // Pre-condition
        // Cannot activate current running fiber
        BOOST_ASSERT(fiber_object::current_fiber_!=this_fiber.get());
        
        this_fiber->state_=fiber_object::READY;
        this_fiber->one_step();
    }
    
    void fiber_object::activate() {
        get_fiber_strand().dispatch(std::bind(activate_fiber, shared_from_this()));
    }
    
    void fiber_object::resume() {
        get_fiber_strand().post(std::bind(activate_fiber, shared_from_this()));
    }
    
    // Following functions can only be called inside coroutine
    void fiber_object::yield() {
        // Pre-condition
        // Can only pause current running fiber
        BOOST_ASSERT(current_fiber_==this);
        BOOST_ASSERT(state_==RUNNING);
        
        set_state(READY);
    }

    void fiber_object::join(fiber_ptr_t f) {
        CHECK_CALLER(this);
        std::lock_guard<std::mutex> lock(f->mtx_);
        if (this==f.get()) {
            // The fiber is joining itself
            throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
        } else if (f->state_==STOPPED) {
            // f is already stopped, do nothing
            return;
        } else {
            f->cleanup_queue_.push_back(std::bind(&fiber_object::activate, shared_from_this()));
        }

        { relock_guard<std::mutex> relock(f->mtx_); pause(); }
    }
    
    void propagate_exception(fiber_ptr_t f) {
        std::exception_ptr e;
        if (f->uncaught_exception_ != std::exception_ptr()) {
            // Propagate uncaught exception in f to this fiber
            e=f->uncaught_exception_;
            // Clean uncaught exception in f
            f->uncaught_exception_=std::exception_ptr();
        }
        // throw propagated exception
        if (e != std::exception_ptr()) {
            std::rethrow_exception(e);
        }
    }
    
    void fiber_object::join_and_rethrow(fiber_ptr_t f) {
        CHECK_CALLER(this);
        std::lock_guard<std::mutex> lock(f->mtx_);
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

        { relock_guard<std::mutex> relock(f->mtx_); pause(); }

        // Joining completed, propagate exception from joinee
        propagate_exception(f);
    }

    void fiber_object::sleep_usec(uint64_t usec) {
        // Shortcut
        if (usec==0) {
            return;
        }
        CHECK_CALLER(this);
        timer_t sleep_timer(get_io_service());
        sleep_timer.expires_from_now(std::chrono::microseconds(usec));
        sleep_timer.async_wait(std::bind(&fiber_object::activate, shared_from_this()));

        pause();
    }
    
    void fiber_object::add_cleanup_function(std::function<void()> &&f) {
        std::lock_guard<std::mutex> lock(mtx_);
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
}}}   // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    void fiber::start() {
        impl_=scheduler::get_instance().impl_->make_fiber(data_);
    }
    
    void fiber::start(attributes attr) {
        if (detail::fiber_object::current_fiber_) {
            // use current scheduler if we're in a fiber
            switch(attr.policy) {
                case attributes::scheduling_policy::normal: {
                    // Create an isolated fiber
                    impl_=detail::fiber_object::current_fiber_->sched_->make_fiber(data_);
                    break;
                }
                case attributes::scheduling_policy::stick_with_parent: {
                    // Create a fiber shares strand with parent
                    impl_=scheduler::get_instance().impl_->make_fiber(detail::fiber_object::current_fiber_->fiber_strand_,
                                                                      data_);
                    break;
                }
                default:
                    break;
            }
        } else {
            // use default scheduler if we're not in a fiber
            impl_=scheduler::get_instance().impl_->make_fiber(data_);
        }
    }
    
    fiber::fiber(fiber &&other) noexcept
    : data_(std::move(other.data_))
    , impl_(std::move(other.impl_))
    {}
    
    fiber& fiber::operator=(fiber &&other) noexcept {
        if (joinable()) {
            // This fiber is still active, std::thread will call std::terminate in the case
            std::terminate();
        }
        impl_=std::move(other.impl_);
        return *this;
    }
    
    void fiber::set_name(const std::string &s) {
        impl_->set_name(s);
    }
    
    std::string fiber::get_name() {
        return impl_->get_name();
    }
    
    bool fiber::joinable() const noexcept {
        // Return true iff this is a fiber and not the current calling fiber
        return (impl_ && detail::fiber_object::current_fiber_!=impl_.get());
    }
    
    fiber::id fiber::get_id() const noexcept {
        return reinterpret_cast<fiber::id>(impl_.get());
    }
    
    void fiber::join(bool propagate_exception) {
        if (!impl_) {
            throw fiber_exception(boost::system::errc::no_such_process);
        }
        if (impl_.get()==detail::fiber_object::current_fiber_) {
            throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
        }
        if (!joinable()) {
            throw invalid_argument();
        }
        if (detail::fiber_object::current_fiber_) {
            if (propagate_exception) {
                detail::fiber_object::current_fiber_->join_and_rethrow(impl_);
            } else {
                detail::fiber_object::current_fiber_->join(impl_);
            }
        }
    }
    
    void fiber::detach() {
        if (!joinable()) {
            throw fiber_exception(boost::system::errc::no_such_process);
        }
        detail::fiber_ptr_t this_fiber=impl_;
        impl_->get_fiber_strand().post(std::bind(&detail::fiber_object::detach, impl_));
        impl_.reset();
    }
    
    void fiber::swap(fiber &other) noexcept(true) {
        std::swap(impl_, other.impl_);
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
                    return ::fibio::fibers::detail::fiber_object::current_fiber_->get_io_service();
                }
                throw fiber_exception(boost::system::errc::no_such_process);
            }
        }   // End of namespace detail
    }   // End of namespace this_fiber
}}  // End of namespace fibio::fibers

