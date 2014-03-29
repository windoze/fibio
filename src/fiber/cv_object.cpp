//
//  cv_object.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include "cv_object.hpp"
#include <boost/system/error_code.hpp>

namespace fibio { namespace fibers { namespace detail {
    void condition_variable_object::wait(mutex_ptr_t m, fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber!=m->owner_) {
            // This fiber doesn't own the mutex
            throw condition_error(boost::system::errc::operation_not_permitted);
        }
        {
            std::lock_guard<std::mutex> lock(m_);
            suspended_.push_back(suspended_item({m, this_fiber, timer_ptr_t()}));
            m->raw_unlock(this_fiber);
        }
        this_fiber->pause();
        m->lock(this_fiber);
    }
    
    cv_status condition_variable_object::wait_usec(mutex_ptr_t m, fiber_ptr_t this_fiber, uint64_t usec) {
        //CHECK_CALLER(this_fiber);
        cv_status ret=cv_status::no_timeout;
        if (this_fiber!=m->owner_) {
            // This fiber doesn't own the mutex
            throw condition_error(boost::system::errc::operation_not_permitted);
        }
        {
            std::lock_guard<std::mutex> lock(m_);
            timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
            suspended_.push_back(suspended_item({m, this_fiber, t}));
            std::shared_ptr<condition_variable_object> this_cv(shared_from_this());
            t->expires_from_now(std::chrono::microseconds(usec));
            t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_cv, t, &ret](boost::system::error_code ec){
                if(!ec) {
                    // Timeout
                    // Timeout handler, find and remove this fiber from waiting queue
                    std::lock_guard<std::mutex> lock(this_cv->m_);
                    ret=cv_status::timeout;
                    // Find and remove this fiber from waiting queue
                    auto i=std::find_if(this_cv->suspended_.begin(),
                                        this_cv->suspended_.end(),
                                        [this_fiber](const suspended_item &i)->bool{
                                            return i.f_==this_fiber;
                                        });
                    if (i!=this_cv->suspended_.end()) {
                        this_cv->suspended_.erase(i);
                    }
                }
                this_fiber->schedule();
            }));
            m->raw_unlock(this_fiber);
        }
        this_fiber->pause();
        m->lock(this_fiber);
        return ret;
    }

    void condition_variable_object::notify_one() {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (suspended_.empty()) {
                return;
            }
            suspended_item p(suspended_.front());
            suspended_.pop_front();
            if (p.t_) {
                // Cancel attached timer if it's set
                // Timer handler will reschedule the waiting fiber
                p.t_->cancel();
                p.t_.reset();
            } else {
                // No timer attached to the waiting fiber, directly schedule it
                p.f_->schedule();
            }
        }
        // Only yield if currently in a fiber
        // CV can be used to notify a fiber from not-a-fiber, i.e. foreign thread
        if (fiber_object::current_fiber_) {
            fiber_object::current_fiber_->yield();
        }
    }
    
    void condition_variable_object::notify_all() {
        {
            std::lock_guard<std::mutex> lock(m_);
            while (!suspended_.empty()) {
                suspended_item p(suspended_.front());
                suspended_.pop_front();
                if (p.t_) {
                    // Cancel attached timer if it's set
                    // Timer handler will reschedule the waiting fiber
                    p.t_->cancel();
                } else {
                    // No timer attached to the waiting fiber, directly schedule it
                    p.f_->schedule();
                }
            }
        }
        // Only yield if currently in a fiber
        // CV can be used to notify a fiber from not-a-fiber, i.e. foreign thread
        if (fiber_object::current_fiber_) {
            fiber_object::current_fiber_->yield();
        }
    }
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    condition_variable::condition_variable()
    : m_(std::make_shared<detail::condition_variable_object>())
    {}
    
    void condition_variable::wait(std::unique_lock<mutex>& lock) {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            m_->wait(lock.mutex()->m_, detail::fiber_object::current_fiber_->shared_from_this());
        }
    }
    
    cv_status condition_variable::wait_usec(std::unique_lock<mutex>& lock, uint64_t usec) {
        CHECK_CURRENT_FIBER;
        if (detail::fiber_object::current_fiber_) {
            return m_->wait_usec(lock.mutex()->m_, detail::fiber_object::current_fiber_->shared_from_this(), usec);
        }
        return cv_status::timeout;
    }
    
    void condition_variable::notify_one() {
        m_->notify_one();
    }
    
    void condition_variable::notify_all() {
        m_->notify_all();
    }
    
    void notify_all_at_thread_exit(condition_variable &cond,
                                   std::unique_lock<mutex> lk)
    {
        CHECK_CURRENT_FIBER;
        struct cleanup_handler {
            condition_variable &c_;
            std::unique_lock<mutex> l_;
            
            cleanup_handler(condition_variable &cond,
                            std::unique_lock<mutex> lk)
            : c_(cond)
            , l_(std::move(lk))
            {}
            
            void operator()() {
                l_.unlock();
                c_.notify_all();
            }
        };
        
        if (detail::fiber_object::current_fiber_) {
            cleanup_handler *h=new cleanup_handler(cond, std::move(lk));
            detail::fiber_object::current_fiber_->add_cleanup_function([&](){
                (*h)();
                delete h;
            });
        }
    }
}}  // End of namespace fibio::fibers
