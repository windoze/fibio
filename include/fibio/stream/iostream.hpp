//
//  iostream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_socket_stream_hpp
#define fibio_socket_stream_hpp

#include <fibio/io/io.hpp>
#include <fibio/stream/basic_iostream.hpp>

namespace fibio { namespace stream {
    typedef basic_iostream<fibio::io::tcp::socket> tcp_stream;
    typedef basic_iostream<fibio::io::posix::stream_descriptor> posix_stream;
}}  // End of namespace fibio::stream

#endif
