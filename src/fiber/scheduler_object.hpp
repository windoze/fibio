//
//  scheduler_object.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-5.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio__scheduler_object__
#define __fibio__scheduler_object__

#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <boost/asio/io_service.hpp>
#include "fiber_object.hpp"

namespace fibio { namespace fibers { namespace detail {
    struct scheduler_object : std::enable_shared_from_this<scheduler_object> {
        scheduler_object();
        fiber_ptr_t make_fiber(fiber_data_ptr entry);
        fiber_ptr_t make_fiber(std::shared_ptr<boost::asio::strand> s, fiber_data_ptr entry);
        void start(size_t nthr);
        void join();
        
        void add_thread(size_t nthr);
        
        void on_fiber_exit(fiber_ptr_t p);
        void on_check_timer(timer_ptr_t check_timer, boost::system::error_code ec);
        
        static std::shared_ptr<scheduler_object> get_instance();
        
        std::mutex mtx_;
        std::condition_variable cv_;
        std::vector<std::thread> threads_;
        boost::asio::io_service io_service_;
        std::atomic<size_t> fiber_count_;
        std::atomic<bool> started_;
        
        static std::once_flag instance_inited_;
        static std::shared_ptr<scheduler_object> the_instance_;
    };
}}} // End of namespace fibio::fibers::detail

#endif /* defined(__fibio__scheduler_object__) */
