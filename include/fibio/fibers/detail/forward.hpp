//
//  forward.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_forward_hpp
#define fibio_fibers_detail_forward_hpp

#include <boost/asio/io_service.hpp>

namespace fibio { namespace fibers { namespace detail {
    struct scheduler_object;
    struct fiber_object;
    struct mutex_object;
    struct recursive_mutex_object;
    struct timed_mutex_object;
    struct recursive_timed_mutex_object;
    struct condition_variable_object;
}}} // End of namespace fibio::fibers::detail

namespace fibio { namespace fibers { namespace this_fiber { namespace detail {
    boost::asio::io_service &get_io_service();
}}}} // End of namespace fibio::fibers::this_fiber::detail

#endif
