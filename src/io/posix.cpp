//
//  posix.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/posix/stream_descriptor.hpp>
#include "io_ops.hpp"

namespace fibio { namespace io {
    using namespace asio;
    using asio::posix::stream_descriptor;
    
    posix::stream_descriptor open(const stream_descriptor::native_handle_type &h) {
        return stream_descriptor(fibers::this_fiber::detail::get_io_service(), h);
    }
    
    size_t read_some(stream_descriptor &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout) {
        return detail::read_some<stream_descriptor>(s, buffer, sz, timeout);
    }
    
    size_t write_some(stream_descriptor &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout) {
        return detail::write_some<stream_descriptor>(s, buffer, sz, timeout);
    }
    
    std::error_code open(stream_descriptor &s, const stream_descriptor::native_handle_type &h) {
        std::error_code ec;
        s.assign(h, ec);
        return ec;
    }
    
    size_t read_some(stream_descriptor &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout,
                     std::error_code &ec) {
        return detail::read_some<stream_descriptor>(s, buffer, sz, timeout, ec);
    }
    
    size_t write_some(stream_descriptor &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout,
                      std::error_code &ec) {
        return detail::write_some<stream_descriptor>(s, buffer, sz, timeout, ec);
    }
}}  // End of namespace fibio::io
