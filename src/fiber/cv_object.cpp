//
//  cv_object.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include "cv_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    void condition_variable_object::wait(mutex_ptr_t m, fiber_ptr_t this_fiber) {
        CHECK_CALLER(this_fiber);
        if (this_fiber->caller_) {
            std::shared_ptr<condition_variable_object> this_cv(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_cv, m](){
                assert(this_fiber==m->owner_);
                std::lock_guard<std::mutex> lock(this_cv->m_);
                this_fiber->state_=fiber_object::BLOCKED;
                this_cv->suspended_.push_back(suspended_item({m, this_fiber, timer_ptr_t()}));
                {
                    // Release mutex
                    std::lock_guard<std::mutex> lock(m->m_);
                    if (m->owner_==this_fiber) {
                        // This fiber owns the mutex
                        if (m->suspended_.empty()) {
                            // Nobody is waiting
                            m->owner_.reset();
                        } else {
                            // There are fibers in the waiting queue, pop one to owner_ and enable its scheduling
                            std::swap(m->owner_, m->suspended_.front());
                            m->suspended_.pop_front();
                            m->owner_->schedule();
                        }
                    } else {
                        // ERROR: This fiber doesn't own the mutex
                        this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                    }
                }

            });
        }
    }
    
    cv_status condition_variable_object::wait_usec(mutex_ptr_t m, fiber_ptr_t this_fiber, uint64_t usec) {
        //CHECK_CALLER(this_fiber);
        cv_status ret=cv_status::no_timeout;
        if (this_fiber->caller_) {
            std::shared_ptr<condition_variable_object> this_cv(shared_from_this());
            (*(this_fiber->caller_))([this_fiber, this_cv, m, usec, &ret](){
                std::lock_guard<std::mutex> lock(this_cv->m_);
                timer_ptr_t t(std::make_shared<timer_t>(this_fiber->io_service_));
                this_cv->suspended_.push_back(suspended_item({m, this_fiber, t}));
                t->expires_from_now(std::chrono::microseconds(usec));
                t->async_wait(this_fiber->fiber_strand_.wrap([this_fiber, this_cv, t, &ret](std::error_code ec){
                    if(ec) {
                        // Timer canceled, wait successful
                        this_fiber->last_error_=ec;
                        return;
                    }
                    // Timeout
                    // Timeout handler, find and remove this fiber from waiting queue
                    std::lock_guard<std::mutex> lock(this_cv->m_);
                    ret=cv_status::timeout;
                    suspended_item p;
                    // Find and remove this fiber from waiting queue
                    std::deque<suspended_item>::iterator i=std::find_if(this_cv->suspended_.begin(),
                                                                        this_cv->suspended_.end(),
                                                                        [this_fiber](const suspended_item &i)->bool{
                                                                            return i.f_==this_fiber;
                                                                        });
                    if (i!=this_cv->suspended_.end()) {
                        std::swap(p, *i);
                        this_cv->suspended_.erase(i);
                        // std::cout << "waiting queue size: " << this_cv->suspended_.size() << std::endl;
                    }
                    if (p.m_) {
                        // Acquire lock for this fiber befor re-scheduling
                        std::lock_guard<std::mutex> lock(p.m_->m_);
                        if (p.m_->owner_) {
                            if (p.m_->owner_==p.f_) {
                                // ERROR: Shouldn't happen
                            } else {
                                // p.mutex is lock by someone else, add fiber to waiting queue
                                p.m_->suspended_.push_back(p.f_);
                                p.f_->state_=fiber_object::BLOCKED;
                            }
                        } else {
                            // p.mutex is not locked, just lock it for p.fiber
                            p.m_->owner_=p.f_;
                            p.f_->schedule();
                        }
                    }
                }));
                this_fiber->state_=fiber_object::BLOCKED;
                //m->unlock(this_fiber);
                {
                    mutex_ptr_t this_mutex=m;
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
                    } else {
                        // ERROR: This fiber doesn't own the mutex
                        this_fiber->last_error_=std::make_error_code(std::errc::operation_not_permitted);
                    }
                }
            });
        }
        return ret;
    }
    
    void condition_variable_object::notify_one() {
        std::lock_guard<std::mutex> lock(m_);
        if (suspended_.empty()) {
            return;
        }
        suspended_item p(suspended_.front());
        suspended_.pop_front();
        if (p.t_) {
            // Cancel attached timer if it's set
            p.t_->cancel();
            p.t_.reset();
        }
        p.f_->fiber_strand_.post([p](){
            // Acquire lock for p.fiber
            std::lock_guard<std::mutex> lock(p.m_->m_);
            if (p.m_->owner_) {
                if (p.m_->owner_==p.f_) {
                    // ERROR: Shouldn't happen
                } else {
                    // p.mutex is lock by someone else, add fiber to waiting queue
                    p.m_->suspended_.push_back(p.f_);
                    p.f_->state_=fiber_object::BLOCKED;
                }
            } else {
                // p.mutex is not locked, just lock it for p.fiber
                p.m_->owner_=p.f_;
                p.f_->schedule();
            }
        });
        fiber_object::current_fiber_->yield();
    }
    
    void condition_variable_object::notify_all() {
        std::lock_guard<std::mutex> lock(m_);
        while (!suspended_.empty()) {
            suspended_item p(suspended_.front());
            suspended_.pop_front();
            if (p.t_) {
                // Cancel attached timer if it's set
                p.t_->cancel();
            }
            p.f_->fiber_strand_.post([p](){
                // Acquire lock p.first for p->second
                std::lock_guard<std::mutex> lock(p.m_->m_);
                if (p.m_->owner_) {
                    if (p.m_->owner_==p.f_) {
                        // ERROR: Shouldn't happen
                    } else {
                        // p.first is lock by someone else
                        p.m_->suspended_.push_back(p.f_);
                        p.f_->state_=fiber_object::BLOCKED;
                    }
                } else {
                    // p.first is not locked, just lock it for p.second
                    p.m_->owner_=p.f_;
                    p.f_->schedule();
                }
            });
        }
        fiber_object::current_fiber_->yield();
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
