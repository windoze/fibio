//
//  forward.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_forward_hpp
#define fibio_fibers_detail_forward_hpp

#include <chrono>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_service.hpp>

namespace fibio { namespace fibers { namespace detail {
    typedef std::chrono::steady_clock::duration duration_t;
    typedef std::chrono::steady_clock::time_point time_point_t;
    typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_t;
    struct scheduler_object;
    struct fiber_object;
    typedef std::shared_ptr<fiber_object> fiber_ptr_t;
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    boost::asio::io_service &get_io_service();
}}}} // End of namespace fibio::fibers::this_fiber::detail

#endif
