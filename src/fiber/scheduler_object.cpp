//
//  scheduler_object.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-5.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/fibers/fiber.hpp>
#include "scheduler_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    std::once_flag scheduler_object::instance_inited_;
    std::shared_ptr<scheduler_object> scheduler_object::the_instance_;
    
    scheduler_object::scheduler_object()
    : fiber_count_(0)
    , started_(false)
    {}
    
    fiber_ptr_t scheduler_object::make_fiber(fiber_data_ptr entry) {
        std::lock_guard<std::mutex> guard(mtx_);
        fiber_count_++;
        fiber_ptr_t ret(std::make_shared<fiber_object>(shared_from_this(), std::move(entry)));
        if (!started_) {
            started_=true;
        }
        ret->resume();
        return ret;
    }
    
    fiber_ptr_t scheduler_object::make_fiber(std::shared_ptr<boost::asio::strand> s, fiber_data_ptr entry) {
        std::lock_guard<std::mutex> guard(mtx_);
        fiber_count_++;
        fiber_ptr_t ret(std::make_shared<fiber_object>(shared_from_this(), s, std::move(entry)));
        if (!started_) {
            started_=true;
        }
        ret->resume();
        return ret;
    }
    
    static inline void run_in_this_thread(scheduler_ptr_t pthis) {
        pthis->io_service_.run();
    }
    
    void scheduler_object::start(size_t nthr) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (threads_.size()>0) {
            // Already started
            return;
        }

        check_timer.reset(new timer_t(io_service_));
        check_timer->expires_from_now(std::chrono::milliseconds(50));
        scheduler_ptr_t pthis(shared_from_this());
        check_timer->async_wait(std::bind(&scheduler_object::on_check_timer, pthis, std::placeholders::_1));
        for(size_t i=0; i<nthr; i++) {
            threads_.push_back(std::thread(std::bind(run_in_this_thread, pthis)));
        }
    }
    
    void scheduler_object::join() {
        {
            // Wait until there is no running fiber
            std::unique_lock<std::mutex> lock(mtx_);
            while (started_ && fiber_count_>0) {
                cv_.wait(lock);
            }
        }
        
        // Join all worker threads
        for(std::thread &t : threads_) {
            t.join();
        }
        threads_.clear();
        started_=false;
        io_service_.reset();
    }
    
    void scheduler_object::add_thread(size_t nthr) {
        std::lock_guard<std::mutex> guard(mtx_);
        scheduler_ptr_t pthis(shared_from_this());
        for(size_t i=0; i<nthr; i++) {
            threads_.push_back(std::thread(std::bind(run_in_this_thread, pthis)));
        }
    }
    
    void scheduler_object::on_fiber_exit(fiber_ptr_t p) {
        std::lock_guard<std::mutex> guard(mtx_);
        fiber_count_--;
        // Release this_ref for detached fibers
        p->this_ref_.reset();
    }
    
    void scheduler_object::on_check_timer(boost::system::error_code ec) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (fiber_count_>0 || !started_) {
            check_timer->expires_from_now(std::chrono::milliseconds(50));
            check_timer->async_wait(std::bind(&scheduler_object::on_check_timer, shared_from_this(), std::placeholders::_1));
        } else {
            io_service_.stop();
            cv_.notify_one();
        }
    }
    
    static inline void init_instance() {
        scheduler_object::the_instance_=std::make_shared<scheduler_object>();
    }

    std::shared_ptr<scheduler_object> scheduler_object::get_instance() {
        std::call_once(instance_inited_, init_instance);
        return scheduler_object::the_instance_;
    }
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers {
    scheduler::scheduler()
    : impl_(std::make_shared<detail::scheduler_object>())
    {}
    
    scheduler::scheduler(std::shared_ptr<detail::scheduler_object> impl)
    : impl_(impl)
    {}
    
    boost::asio::io_service &scheduler::get_io_service()
    { return impl_->io_service_; }
    
    void scheduler::start(size_t nthr) {
        impl_->start(nthr);
    }
    
    void scheduler::join() {
        impl_->join();
    }
    
    void scheduler::add_worker_thread(size_t nthr) {
        impl_->add_thread(nthr);
    }
    
    scheduler scheduler::get_instance() {
        return scheduler(detail::scheduler_object::get_instance());
    }
    
    void scheduler::reset_instance() {
        if (detail::scheduler_object::the_instance_) {
            return detail::scheduler_object::the_instance_.reset();
        }
    }
}}  // End of namespace fibio::fibers
