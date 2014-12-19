//
//  condition.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/system/error_code.hpp>
#include <fibio/fibers/condition_variable.hpp>
#include "fiber_object.hpp"

namespace fibio { namespace fibers {
    static const auto NOPERM=condition_error(boost::system::errc::operation_not_permitted);
    
    void condition_variable::wait(std::unique_lock<mutex>& lock) {
        auto tf=current_fiber_ptr();
        mutex *m=lock.mutex();
        if (tf!=m->owner_) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        {
            std::lock_guard<detail::spinlock> lock(mtx_);
            // The "suspension of this fiber" is actually happened here, not the pause()
            // as other will see there is a fiber in the waiting queue.
            suspended_.push_back(suspended_item({m, tf, 0}));
        }
        { detail::relock_guard<mutex> relock(*m); tf->pause(); }
    }
    
    void condition_variable::timeout_handler(detail::fiber_ptr_t this_fiber,
                                             detail::timer_t *t,
                                             cv_status &ret,
                                             boost::system::error_code ec)
    {
        if(!ec) {
            // Timeout
            // Timeout handler, find and remove this fiber from waiting queue
            std::lock_guard<detail::spinlock> lock(mtx_);
            ret=cv_status::timeout;
            // Find and remove this fiber from waiting queue
            auto i=std::find(suspended_.begin(),
                             suspended_.end(),
                             this_fiber);
            if (i!=suspended_.end()) {
                suspended_.erase(i);
            }
        }
        this_fiber->resume();
    }
    
    cv_status condition_variable::wait_rel(std::unique_lock<mutex>& lock, detail::duration_t d) {
        auto tf=current_fiber_ptr();
        mutex *m=lock.mutex();
        cv_status ret=cv_status::no_timeout;
        if (tf!=m->owner_) {
            // This fiber doesn't own the mutex
            BOOST_THROW_EXCEPTION(NOPERM);
        }
        detail::timer_t t(tf->get_io_service());
        {
            std::lock_guard<detail::spinlock> lock(mtx_);
            suspended_.push_back(suspended_item({m, tf, &t}));
            t.expires_from_now(d);
            t.async_wait(tf->get_fiber_strand().wrap(std::bind(&condition_variable::timeout_handler,
                                                                        this,
                                                                        tf,
                                                                        &t,
                                                                        std::ref(ret),
                                                                        std::placeholders::_1)));
        }
        { detail::relock_guard<mutex> relock(*m); tf->pause(); }
        return ret;
    }
    
    void condition_variable::notify_one() {
        {
            std::lock_guard<detail::spinlock> lock(mtx_);
            if (suspended_.empty()) {
                return;
            }
            suspended_item p(suspended_.front());
            suspended_.pop_front();
            if (p.t_) {
                // Cancel attached timer if it's set
                // Timer handler will reschedule the waiting fiber
                p.t_->cancel();
                p.t_=0;
            } else {
                // No timer attached to the waiting fiber, directly schedule it
                p.f_->resume();
            }
        }
        // Only yield if currently in a fiber
        // CV can be used to notify a fiber from not-a-fiber, i.e. foreign thread
        if (auto cf=current_fiber()) {
            cf->yield();
        }
    }
    
    void condition_variable::notify_all() {
        {
            std::lock_guard<detail::spinlock> lock(mtx_);
            while (!suspended_.empty()) {
                suspended_item p(suspended_.front());
                suspended_.pop_front();
                if (p.t_) {
                    // Cancel attached timer if it's set
                    // Timer handler will reschedule the waiting fiber
                    p.t_->cancel();
                } else {
                    // No timer attached to the waiting fiber, directly schedule it
                    p.f_->resume();
                }
            }
        }
        // Only yield if currently in a fiber
        // CV can be used to notify a fiber from not-a-fiber, i.e. foreign thread
        if (auto cf=current_fiber()) {
            cf->yield();
        }
    }

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
    
    inline void run_cleanup_handler(cleanup_handler *h) {
        std::unique_ptr<cleanup_handler> ch(h);
        (*ch)();
    }
    
    void notify_all_at_fiber_exit(condition_variable &cond,
                                   std::unique_lock<mutex> lk)
    {
        CHECK_CURRENT_FIBER;
        if (auto cf=current_fiber()) {
            cleanup_handler *h=new cleanup_handler(cond, std::move(lk));
            cf->add_cleanup_function(std::bind(run_cleanup_handler, h));
        }
    }
}}  // End of namespace fibio::fibers
