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
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_==this_fiber) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
            } else if(!owner_) {
                // This mutex is not locked
                // Acquire the mutex
                owner_=this_fiber;
                return;
            }
            // This mutex is locked
            // Add this fiber into waiting queue
            suspended_.push_back(this_fiber);
        }
        this_fiber->pause();
    }
    
    void mutex_object::raw_unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<std::mutex> lock(m_);
        if (owner_!=this_fiber) {
            // This fiber doesn't own the mutex
            throw fiber_exception(boost::system::errc::operation_not_permitted);
        }
        if (suspended_.empty()) {
            // Nobody is waiting
            owner_.reset();
            return;
        }
        // Set new owner and remove it from suspended queue
        std::swap(owner_, suspended_.front());
        suspended_.pop_front();
        owner_->schedule();
    }
    
    void mutex_object::unlock(fiber_ptr_t this_fiber) {
        raw_unlock(this_fiber);
        this_fiber->yield();
    }
    
    bool mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<std::mutex> lock(m_);
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
        {
            std::lock_guard<std::mutex> lock(m_);
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
        }
        this_fiber->pause();
    }

    void recursive_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_!=this_fiber) {
                // This fiber doesn't own the mutex
                throw fiber_exception(boost::system::errc::operation_not_permitted);
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
            owner_->schedule();
        }
        this_fiber->yield();
    }
    
    bool recursive_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<std::mutex> lock(m_);
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
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_==this_fiber) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
            } else if(!owner_) {
                // This mutex is not locked
                // Acquire the mutex
                owner_=this_fiber;
                return;
            }
            // This mutex is locked
            // Add this fiber into waiting queue without attached timer
            suspended_.push_back({this_fiber, timer_ptr_t()});
        }
        this_fiber->pause();
    }

    bool timed_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<std::mutex> lock(m_);
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
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_!=this_fiber) {
                // This fiber doesn't own the mutex
                throw fiber_exception(boost::system::errc::operation_not_permitted);
            }
            if (suspended_.empty()) {
                // Nobody is waiting
                owner_.reset();
                return;
            }
            // Set new owner and remove it from suspended queue
            std::swap(owner_, suspended_.front().f_);
            timer_ptr_t t=suspended_.front().t_;
            suspended_.pop_front();
            if (t) {
                // Cancel attached timer, the timer handler will schedule new owner
                t->cancel();
            } else {
                // No attached timer, directly schedule new owner
                owner_->schedule();
            }
        }
        this_fiber->yield();
    }

    bool timed_mutex_object::try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec) {
        CHECK_CALLER(this_fiber);
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_==this_fiber) {
                throw fiber_exception(boost::system::errc::resource_deadlock_would_occur);
            } else if(!owner_) {
                // This mutex is not locked
                // Acquire the mutex
                owner_=this_fiber;
                return true;
            }
            // This mutex is locked
            // Add this fiber into waiting queue
            timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
            t->expires_from_now(std::chrono::microseconds(usec));
            std::shared_ptr<timed_mutex_object> this_mutex=shared_from_this();
            t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_mutex](boost::system::error_code ec){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_!=this_fiber) {
                    // This fiber doesn't own the mutex
                    // Find and remove this fiber from waiting queue
                    auto i=std::find_if(this_mutex->suspended_.begin(),
                                        this_mutex->suspended_.end(),
                                        [this_fiber](const suspended_item &i)->bool{
                                            return i.f_==this_fiber;
                                        });
                    if (i!=this_mutex->suspended_.end()) {
                        this_mutex->suspended_.erase(i);
                    }
                } else {
                    // This fiber should not be in the suspended queue, do nothing
                }
                this_fiber->schedule();
            }));
            suspended_.push_back({this_fiber, t});
        }
        this_fiber->pause();
        // This fiber will be resumed when timer triggered/canceled or other called unlock()
        return owner_==this_fiber;
    }

    void timed_recursive_mutex_object::lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        {
            std::lock_guard<std::mutex> lock(m_);
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
            suspended_.push_back({this_fiber, timer_ptr_t()});
        }
        this_fiber->pause();
    }

    void timed_recursive_mutex_object::unlock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        {
            std::lock_guard<std::mutex> lock(m_);
            if (owner_!=this_fiber) {
                // This fiber doesn't own the mutex
                throw fiber_exception(boost::system::errc::operation_not_permitted);
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
            timer_ptr_t t=suspended_.front().t_;
            suspended_.pop_front();
            level_=1;
            if (t) {
                // Cancel attached timer, the timer handler will schedule new owner
                t->cancel();
            } else {
                // No attached timer, directly schedule new owner
                owner_->schedule();
            }
        }
        this_fiber->yield();
    }
    
    bool timed_recursive_mutex_object::try_lock(fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        std::lock_guard<std::mutex> lock(m_);
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

    bool timed_recursive_mutex_object::try_lock_usec(fiber_ptr_t this_fiber, uint64_t usec) {
        CHECK_CALLER(this_fiber);
        {
            std::lock_guard<std::mutex> lock(m_);
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
            timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
            t->expires_from_now(std::chrono::microseconds(usec));
            std::shared_ptr<timed_recursive_mutex_object> this_mutex=shared_from_this();
            t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_mutex](boost::system::error_code ec){
                std::lock_guard<std::mutex> lock(this_mutex->m_);
                if (this_mutex->owner_!=this_fiber) {
                    // This fiber doesn't own the mutex
                    // Find and remove this fiber from waiting queue
                    auto i=std::find_if(this_mutex->suspended_.begin(),
                                        this_mutex->suspended_.end(),
                                        [this_fiber](const suspended_item &i)->bool{
                                            return i.f_==this_fiber;
                                        });
                    if (i!=this_mutex->suspended_.end()) {
                        this_mutex->suspended_.erase(i);
                    }
                } else {
                    // This fiber should not be in the suspended queue, do nothing
                }
                this_fiber->schedule();
            }));
            suspended_.push_back({this_fiber, t});
        }
        this_fiber->pause();
        // This fiber will be resumed when timer triggered/canceled or other called unlock()
        return owner_==this_fiber;
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
