//
//  mutex_object.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/fibers/mutex.hpp>
#include "mutex_object.hpp"


namespace fibio { namespace fibers { namespace detail {
    static const auto NOPERM=lock_error(boost::system::errc::operation_not_permitted);
    static const auto DEADLOCK=lock_error(boost::system::errc::resource_deadlock_would_occur);

    void mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            return;
        }
        // This mutex is locked
        // Add this fiber into waiting queue
        suspended_.push_back(this_fiber);

        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }
    }
    
    void mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front());
        suspended_.pop_front();
        owner_->resume();

        { relock_guard<spinlock> relock(mtx_); this_fiber->yield(owner_); }
    }
    
    bool mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            // This fiber already owns the mutex
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
        }
        // Return true if this fiber owns the mutex
        return owner_==this_fiber;
    }
    
    void recursive_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            ++level_;
            return;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            assert(suspended_.empty());
            assert(level_==0);
            owner_=this_fiber;
            level_=1;
            return;
        }
        // This mutex is locked
        // Add this fiber into waiting queue
        suspended_.push_back(this_fiber);
        
        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }
    }

    void recursive_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        --level_;
        if (level_>0) {
            // This fiber still owns the mutex
            return;
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            assert(level_==0);
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front());
        suspended_.pop_front();
        level_=1;
        owner_->resume();
        
        { relock_guard<spinlock> relock(mtx_); this_fiber->yield(owner_); }
    }
    
    bool recursive_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            // This fiber already owns the mutex, increase recursive level
            ++level_;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            assert(suspended_.empty());
            assert(level_==0);
            owner_=this_fiber;
            level_=1;
        }
        // Cannot acquire the lock now
        return owner_==this_fiber;
    }

    void timed_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            return;
        }
        // This mutex is locked
        // Add this fiber into waiting queue without attached timer
        suspended_.push_back({this_fiber, 0});
        
        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }
    }

    bool timed_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            // This fiber already owns the mutex
            return true;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            return true;
        }
        // Cannot acquire the lock now
        return false;
    }
    
    void timed_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front().f_);
        timer_t *t=suspended_.front().t_;
        suspended_.pop_front();
        if (t) {
            // Cancel attached timer, the timer handler will schedule new owner
            t->cancel();
        } else {
            // No attached timer, directly schedule new owner
            owner_->resume();
        }
        
        { relock_guard<spinlock> relock(mtx_); this_fiber->yield(owner_); }
    }
    
    static inline void timed_mutex_timeout_handler(fiber_ptr_t this_fiber,
                                                   timed_mutex_object *this_mutex,
                                                   boost::system::error_code ec)
    {
        std::lock_guard<spinlock> lock(this_mutex->mtx_);
        if (this_mutex->owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            // Find and remove this fiber from waiting queue
            auto i=std::find(this_mutex->suspended_.begin(),
                             this_mutex->suspended_.end(),
                             this_fiber);
            if (i!=this_mutex->suspended_.end()) {
                this_mutex->suspended_.erase(i);
            }
        } else {
            // This fiber should not be in the suspended queue, do nothing
        }
        this_fiber->resume();
    }

    bool timed_mutex_object::try_lock_rel(fiber_ptr_t this_fiber, duration_t d) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            BOOST_THROW_EXCEPTION(DEADLOCK);
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            return true;
        }
        // This mutex is locked
        // Add this fiber into waiting queue
        timer_t t(this_fiber->get_io_service());
        t.expires_from_now(d);
        t.async_wait(this_fiber->get_fiber_strand().wrap(std::bind(timed_mutex_timeout_handler,
                                                                    this_fiber,
                                                                    this,
                                                                    std::placeholders::_1)));
        suspended_.push_back({this_fiber, &t});
        
        // This fiber will be resumed when timer triggered/canceled or other called unlock()
        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }

        return owner_==this_fiber;
    }

    void recursive_timed_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            ++level_;
            return;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            level_=1;
            return;
        }
        // This mutex is locked
        // Add this fiber into waiting queue without attached timer
        suspended_.push_back({this_fiber, 0});

        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }
    }

    void recursive_timed_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        --level_;
        if(level_>0) {
            // This fiber still owns the mutex
            return;
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            level_=0;
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front().f_);
        timer_t *t=suspended_.front().t_;
        suspended_.pop_front();
        level_=1;
        if (t) {
            // Cancel attached timer, the timer handler will schedule new owner
            t->cancel();
        } else {
            // No attached timer, directly schedule new owner
            owner_->resume();
        }

        { relock_guard<spinlock> relock(mtx_); this_fiber->yield(owner_); }
    }
    
    bool recursive_timed_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            // This fiber already owns the mutex, increase recursive level
            ++level_;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            level_=1;
        }
        // Cannot acquire the lock now
        return owner_==this_fiber;
    }
    
    static inline void recursive_timed_mutex_timeout_handler(fiber_ptr_t this_fiber,
                                                             recursive_timed_mutex_object *this_mutex,
                                                             boost::system::error_code ec)
    {
        std::lock_guard<spinlock> lock(this_mutex->mtx_);
        if (this_mutex->owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            // Find and remove this fiber from waiting queue
            auto i=std::find(this_mutex->suspended_.begin(),
                             this_mutex->suspended_.end(),
                             this_fiber);
            if (i!=this_mutex->suspended_.end()) {
                this_mutex->suspended_.erase(i);
            }
        } else {
            // This fiber should not be in the suspended queue, do nothing
        }
        this_fiber->resume();
    }

    bool recursive_timed_mutex_object::try_lock_rel(fiber_ptr_t this_fiber, duration_t d) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<spinlock> lock(mtx_);
        if (owner_==this_fiber) {
            ++level_;
            return true;
        } else if(!owner_) {
            // This mutex is not locked
            // Acquire the mutex
            owner_=this_fiber;
            level_=1;
            return true;
        }
        // This mutex is locked
        // Add this fiber into waiting queue
        timer_t t(this_fiber->get_io_service());
        t.expires_from_now(d);
        t.async_wait(this_fiber->get_fiber_strand().wrap(std::bind(recursive_timed_mutex_timeout_handler,
                                                                    this_fiber,
                                                                    this,
                                                                    std::placeholders::_1)));
        suspended_.push_back({this_fiber, &t});

        // This fiber will be resumed when timer triggered/canceled or other called unlock()
        { relock_guard<spinlock> relock(mtx_); this_fiber->pause(); }
        return owner_==this_fiber;
    }
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    mutex::mutex()
    : impl_(new detail::mutex_object)
    {}
    
    void mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->lock(cf->shared_from_this());
        }
    }
    
    void mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->unlock(cf->shared_from_this());
        }
    }
    
    bool mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            return impl_->try_lock(cf->shared_from_this());
        }
        return false;
    }
    
    timed_mutex::timed_mutex()
    : impl_(new detail::timed_mutex_object)
    {}
    
    void timed_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->lock(cf->shared_from_this());
        }
    }
    
    void timed_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->unlock(cf->shared_from_this());
        }
    }
    
    bool timed_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            return impl_->try_lock(cf->shared_from_this());
        }
        return false;
    }
    
    bool timed_mutex::try_lock_rel(detail::duration_t d) {
        CHECK_CURRENT_FIBER;
        if (d<detail::duration_t::zero()) {
            BOOST_THROW_EXCEPTION(fibio::invalid_argument());
        }
        if (auto cf=current_fiber()) {
            return impl_->try_lock_rel(cf->shared_from_this(), d);
        }
        return false;
    }
    
    recursive_mutex::recursive_mutex()
    : impl_(new detail::recursive_mutex_object)
    {}
    
    void recursive_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->lock(cf->shared_from_this());
        }
    }
    
    void recursive_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->unlock(cf->shared_from_this());
        }
    }
    
    bool recursive_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            return impl_->try_lock(cf->shared_from_this());
        }
        return false;
    }
    
    recursive_timed_mutex::recursive_timed_mutex()
    : impl_(new detail::recursive_timed_mutex_object)
    {}
    
    void recursive_timed_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->lock(cf->shared_from_this());
        }
    }
    
    void recursive_timed_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            impl_->unlock(cf->shared_from_this());
        }
    }
    
    bool recursive_timed_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            return impl_->try_lock(cf->shared_from_this());
        }
        return false;
    }
    
    bool recursive_timed_mutex::try_lock_rel(detail::duration_t d) {
        CHECK_CURRENT_FIBER;
        if (d<detail::duration_t::zero()) {
            BOOST_THROW_EXCEPTION(fibio::invalid_argument());
        }
        if (auto cf=current_fiber()) {
            return impl_->try_lock_rel(cf->shared_from_this(), d);
        }
        return false;
    }
    
    void mutex::impl_deleter::operator()(detail::mutex_object *p)
    { delete p; }
    
    void timed_mutex::impl_deleter::operator()(detail::timed_mutex_object *p)
    { delete p; }
    
    void recursive_mutex::impl_deleter::operator()(detail::recursive_mutex_object *p)
    { delete p; }
    
    void recursive_timed_mutex::impl_deleter::operator()(detail::recursive_timed_mutex_object *p)
    { delete p; }
}}  // End of namespace fibio::fibers
