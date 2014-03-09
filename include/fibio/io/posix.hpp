//
//  posix.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_posix_hpp
#define fibio_posix_hpp

#include <asio/posix/stream_descriptor.hpp>

namespace fibio { namespace io {
    namespace posix {
        using asio::posix::stream_descriptor;
    }   // End of namespace fibio::io::posix

    posix::stream_descriptor open(const posix::stream_descriptor::native_handle_type &h);
    size_t read_some(posix::stream_descriptor &s, char *buffer, size_t sz, uint64_t timeout_usec=0);
    size_t write_some(posix::stream_descriptor &s, const char *buffer, size_t sz, uint64_t timeout_usec=0);

    std::error_code open(posix::stream_descriptor &s, const posix::stream_descriptor::native_handle_type &h);
    size_t read_some(posix::stream_descriptor &s, char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
    size_t write_some(posix::stream_descriptor &s, const char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
}}  // End of namespace fibio::io

#endif
