//
//  stream_protocol.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_local_stream_protocol_hpp
#define fibio_io_local_stream_protocol_hpp

#include <boost/asio/local/stream_protocol.hpp>
#include <fibio/io/basic_stream_socket.hpp>
#include <fibio/io/basic_socket_acceptor.hpp>

namespace fibio {
    typedef io::fiberized<boost::asio::local::stream_protocol::socket> local_stream_socket;
    typedef io::fiberized<boost::asio::local::stream_protocol::acceptor> local_stream_socket_acceptor;
}   // End of namespace fibio

#endif
