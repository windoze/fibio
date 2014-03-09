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
    void mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->owner_=this_fiber;
                } else if (this_mutex->owner_==this_fiber) {
                    // ERROR: Deadlock, already own the lock
                    this_fiber->last_error_=std::make_error_code(std::errc::resource_deadlock_would_occur);
                } else {
                    // The mutex is locked, add the fiber to the waiting queue
                    this_fiber->state_=fiber_object::BLOCKED;
                    this_mutex->suspended_.push_back(this_fiber);
                }
#if defined(DEBUG) && !defined(NDEBUG)
                // Invariant check
                {
                    // The lock must has an owner if some other fibers are awaiting
                    if (this_mutex->suspended_.size()>0) {
                        assert(this_mutex->owner_);
                    }
                    // The current owner must not be blocked
                    if (this_mutex->owner_==this_fiber) {
                        assert(this_fiber->state_!=fiber_object::BLOCKED);
                    }
                    // The suspended fiber must be blocked
                    if (this_mutex->owner_!=this_fiber) {
                        assert(this_fiber->state_==fiber_object::BLOCKED);
                    }
                    // If this fiber is blocked, it must not be the owner
                    if (this_fiber->state_==fiber_object::BLOCKED) {
                        assert(this_fiber!=this_mutex->owner_);
                    }
                }
#endif
            });
        }
#if defined(DEBUG) && !defined(NDEBUG)
        // Invariant check
        {
            std::lock_guard<std::mutex> lock(m_);
            // This fiber must not be blocked
            assert(this_fiber->state_!=fiber_object::BLOCKED);
            // If there is no error, then this fiber must own the mutex
            if (!this_fiber->last_error_) {
                assert(this_fiber==owner_);
            }
        }
#endif
        this_fiber->throw_on_error();
    }
    
    void mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_==this_fiber) {
                    // This fiber owns the mutex
                    if (this_mutex->suspended_.empty()) {
                        // Nobody is waiting
                        this_mutex->owner_.reset();
                    } else {
                        // There are fibers in the waiting queue, pop one to owner_ and enable its scheduling
                        std::swap(this_mutex->owner_, this_mutex->suspended_.front());
                        this_mutex->suspended_.pop_front();
                        this_mutex->owner_->schedule();
                    }
                    this_fiber->state_=fiber_object::READY;
                } else {
                    // ERROR: This fiber doesn't own the mutex
                    this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                }
#if defined(DEBUG) && !defined(NDEBUG)
                // Invariant check
                {
                    // The lock must has an owner if some other fibers are awaiting
                    if (this_mutex->suspended_.size()>0) {
                        assert(this_mutex->owner_);
                    }
                    // The current owner must not be blocked
                    if (this_mutex->owner_==this_fiber) {
                        assert(this_fiber->state_!=fiber_object::BLOCKED);
                    }
                    // This fiber must not be the owner
                    assert(this_mutex->owner_ != this_fiber);
                    
                    // This fiber must not be blocked
                    assert(this_fiber->state_!=fiber_object::BLOCKED);
                }
#endif
            });
        }
#if defined(DEBUG) && !defined(NDEBUG)
        // Invariant check
        {
            std::lock_guard<std::mutex> lock(m_);
            // If there is no error, then
            // This fiber must own the mutex
            assert(this_fiber!=owner_);
            // This fiber must not be blocked
            assert(this_fiber->state_!=fiber_object::BLOCKED);
        }
#endif
        this_fiber->throw_on_error();
    }
    
    bool mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        bool ret=false;
        if (this_fiber->caller_) {
            std::shared_ptr<mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([&ret, this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_mutex->owner_=this_fiber;
                    this_fiber->state_=fiber_object::RUNNING;
                    ret=true;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock
                    ret=true;
                }
                ret=false;
            });
        }
        return ret;
    }
    
    void recursive_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->owner_=this_fiber;
                    this_mutex->level_=1;
                } else if (this_mutex->owner_==this_fiber) {
                    // Increase recursive level
                    this_mutex->level_++;
                } else {
                    // The mutex is locked, add the fiber to the waiting queue
                    this_fiber->state_=fiber_object::BLOCKED;
                    this_mutex->suspended_.push_back(this_fiber);
                }
            });
        }
        this_fiber->throw_on_error();
    }
    
    bool recursive_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        bool ret=false;
        if (this_fiber->caller_) {
            std::shared_ptr<recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([&ret, this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->owner_=this_fiber;
                    this_mutex->level_=1;
                    ret=true;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock
                    this_mutex->level_++;
                    ret=true;
                }
                ret=false;
            });
        }
        return ret;
    }
    
    void recursive_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_==this_fiber) {
                    // This fiber owns the mutex
                    this_mutex->level_--;
                    if (this_mutex->level_>0) {
                        // This mutex is not fully unlocked, no need to scheduler others
                        this_fiber->state_=fiber_object::RUNNING;
                        return;
                    }
                    if (this_mutex->suspended_.empty()) {
                        // Nobody is waiting
                        assert(this_mutex->level_==0);
                        this_mutex->owner_.reset();
                    } else {
                        // There are fibers in the waiting queue, pop one to owner_ and enable its scheduling
                        std::swap(this_mutex->owner_, this_mutex->suspended_.front());
                        this_mutex->suspended_.pop_front();
                        this_mutex->owner_->schedule();
                        this_mutex->level_=1;
                    }
                    this_fiber->state_=fiber_object::READY;
                } else {
                    // ERROR: This fiber doesn't own the mutex
                    this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                }
            });
        }
        this_fiber->throw_on_error();
    }
    
    void timed_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<timed_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_mutex->owner_=this_fiber;
                    this_fiber->state_=fiber_object::RUNNING;
                } else if (this_mutex->owner_.get()==this_fiber.get()) {
                    // ERROR: Deadlock, already own the lock
                    this_fiber->last_error_=std::make_error_code(std::errc::resource_deadlock_would_occur);
                } else {
                    // The mutex is locked, add the fiber to the waiting queue
                    this_mutex->suspended_.push_back(suspended_item({this_fiber, timer_ptr_t()}));
                    this_fiber->state_=fiber_object::BLOCKED;
                }
            });
        }
        this_fiber->throw_on_error();
    }
    
    bool timed_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        bool ret=false;
        if (this_fiber->caller_) {
            std::shared_ptr<timed_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([&ret, this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_mutex->owner_=this_fiber;
                    this_fiber->state_=fiber_object::RUNNING;
                    ret=true;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock
                    ret=true;
                }
                ret=false;
            });
        }
        return ret;
    }
    
    bool timed_mutex_object::try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec) {
        CHECK_CALLER(this_fiber);
        bool ret=true;
        if (this_fiber->caller_) {
            // Will block
            std::shared_ptr<timed_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex, usec, &ret](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Shortcut, not locked, don't bother to start the timer
                    this_mutex->owner_=this_fiber;
                    this_fiber->state_=fiber_object::RUNNING;
                    ret=true;
                    return;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock
                    ret=true;
                    return;
                }

                this_fiber->state_=fiber_object::BLOCKED;
                timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
                this_mutex->suspended_.push_back(suspended_item({this_fiber, t}));
                t->expires_from_now(std::chrono::microseconds(usec));
                t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_mutex, t, &ret](std::error_code ec){
                    if(ec) {
                        this_fiber->last_error_=ec;
                        return;
                    }
                    // Timeout
                    ret=false;
                    suspended_item p;
                    // Find and remove this fiber from waiting queue
                    std::lock_guard<std::mutex> lock(this_mutex->m_);
                    std::deque<suspended_item>::iterator i=std::find_if(this_mutex->suspended_.begin(),
                                                                        this_mutex->suspended_.end(),
                                                                        [this_fiber](const suspended_item &i)->bool{
                                                                            return i.f_==this_fiber;
                                                                        });
                    if (i!=this_mutex->suspended_.end()) {
                        // i is pointing to this_fiber
                        assert(i->f_==this_fiber);
                        // Re-schedule suspended fiber if it's in the suspended queue
                        std::swap(p, *i);
                        this_mutex->suspended_.erase(i);
                        p.f_->schedule();
                    }
                }));
            });
        }

        this_fiber->throw_on_error();
        return ret;
    }
    
    void timed_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<timed_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_==this_fiber) {
                    // This fiber owns the mutex
                    if (this_mutex->suspended_.empty()) {
                        // Nobody is waiting
                        this_mutex->owner_.reset();
                    } else {
                        // There are fibers in the waiting queue, pop one to owner_ and enable its scheduling
                        std::swap(this_mutex->owner_, this_mutex->suspended_.front().f_);
                        if (this_mutex->suspended_.front().t_) {
                            // Cancel attached timer
                            this_mutex->suspended_.front().t_->cancel();
                        }
                        this_mutex->suspended_.pop_front();
                        this_mutex->owner_->schedule();
                    }
                    this_fiber->state_=fiber_object::READY;
                } else {
                    // ERROR: This fiber doesn't own the mutex
                    this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                }
            });
        }
        this_fiber->throw_on_error();
    }
    
    void timed_recursive_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<timed_recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->owner_=this_fiber;
                    this_mutex->level_=1;
                } else if (this_mutex->owner_==this_fiber) {
                    // Increase recursive level
                    this_mutex->level_++;
                } else {
                    // The mutex is locked, add the fiber to the waiting queue
                    this_fiber->state_=fiber_object::BLOCKED;
                    this_mutex->suspended_.push_back(suspended_item({this_fiber, timer_ptr_t()}));
                }
            });
        }
        this_fiber->throw_on_error();
    }
    
    bool timed_recursive_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        bool ret=false;
        if (this_fiber->caller_) {
            std::shared_ptr<timed_recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([&ret, this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Not locked
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->owner_=this_fiber;
                    this_mutex->level_=1;
                    ret=true;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock
                    this_mutex->level_++;
                    ret=true;
                }
                ret=false;
            });
        }
        return ret;
    }
    
    bool timed_recursive_mutex_object::try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec) {
        CHECK_CALLER(this_fiber);
        bool ret=true;
        if (this_fiber->caller_) {
            // Will block
            std::shared_ptr<timed_recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex, usec, &ret](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (!this_mutex->owner_) {
                    // Shortcut, not locked, don't bother to start the timer
                    this_mutex->owner_=this_fiber;
                    this_fiber->state_=fiber_object::RUNNING;
                    this_mutex->level_=1;
                    ret=true;
                    return;
                }
                if (this_mutex->owner_==this_fiber) {
                    // Already own the lock, increase recursive level
                    this_mutex->level_++;
                    ret=true;
                    return;
                }

                this_fiber->state_=fiber_object::BLOCKED;
                timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
                this_mutex->suspended_.push_back(suspended_item({this_fiber, t}));
                t->expires_from_now(std::chrono::microseconds(usec));
                t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_mutex, t, &ret](std::error_code ec){
                    if(ec) {
                        this_fiber->last_error_=ec;
                        return;
                    }
                    // Timeout
                    ret=false;
                    suspended_item p;
                    // Find and remove this fiber from waiting queue
                    std::lock_guard<std::mutex> lock(this_mutex->m_);
                    std::deque<suspended_item>::iterator i=std::find_if(this_mutex->suspended_.begin(),
                                                                        this_mutex->suspended_.end(),
                                                                        [this_fiber](const suspended_item &i)->bool{
                                                                            return i.f_==this_fiber;
                                                                        });
                    if (i!=this_mutex->suspended_.end()) {
                        // Re-schedule suspended fiber if it's in the suspended queue
                        std::swap(p, *i);
                        this_mutex->suspended_.erase(i);
                        p.f_->schedule();
                    }
                }));
            });
        }
        this_fiber->throw_on_error();
        return ret;
    }
    
    void timed_recursive_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<timed_recursive_mutex_object> this_mutex(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_mutex](){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_==this_fiber) {
                    // This fiber owns the mutex
                    this_mutex->level_--;
                    if (this_mutex->level_>0) {
                        // This mutex is not fully unlocked, no need to scheduler others
                        this_fiber->state_=fiber_object::RUNNING;
                        return;
                    }
                    if (this_mutex->suspended_.empty()) {
                        // Nobody is waiting
                        assert(this_mutex->level_==0);
                        this_mutex->owner_.reset();
                    } else {
                        // There are fibers in the waiting queue, pop one to owner_ and enable its scheduling
                        std::swap(this_mutex->owner_, this_mutex->suspended_.front().f_);
                        this_mutex->suspended_.pop_front();
                        this_mutex->owner_->schedule();
                        this_mutex->level_=1;
                    }
                    this_fiber->state_=fiber_object::READY;
                } else {
                    // ERROR: This fiber doesn't own the mutex
                    this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                }
            });
        }
        this_fiber->throw_on_error();
    }
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    mutex::mutex()
    : m_(std::make_shared<detail::mutex_object>())
    {}
    
    void mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    void mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->unlock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    bool mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
        return false;
    }
    
    timed_mutex::timed_mutex()
    : m_(std::make_shared<detail::timed_mutex_object>())
    {}
    
    void timed_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    void timed_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->unlock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    bool timed_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
        return false;
    }
    
    bool timed_mutex::try_lock_usec(uint64_t usec) {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock_usec(detail::fiber_object::current_fiber_->shared_from_this(), usec);
        }
        return false;
    }
    
    recursive_mutex::recursive_mutex()
    : m_(std::make_shared<detail::recursive_mutex_object>())
    {}
    
    void recursive_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    void recursive_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->unlock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    bool recursive_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
        return false;
    }
    
    timed_recursive_mutex::timed_recursive_mutex()
    : m_(std::make_shared<detail::timed_recursive_mutex_object>())
    {}
    
    void timed_recursive_mutex::lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    void timed_recursive_mutex::unlock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->unlock(detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    bool timed_recursive_mutex::try_lock() {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock(detail::fiber_object::current_fiber_->shared_from_this());
        }
        return false;
    }
    
    bool timed_recursive_mutex::try_lock_usec(uint64_t usec) {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->try_lock_usec(detail::fiber_object::current_fiber_->shared_from_this(), usec);
        }
        return false;
    }
}}  // End of namespace fibio::fibers
