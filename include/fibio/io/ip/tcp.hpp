//
//  tcp.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_ip_tcp_hpp
#define fibio_io_ip_tcp_hpp

#include <boost/asio/ip/tcp.hpp>
#include <fibio/io/basic_stream_socket.hpp>
#include <fibio/io/basic_socket_acceptor.hpp>
#include <fibio/io/ip/basic_resolver.hpp>

namespace fibio {
    namespace io {
        typedef io::fiberized<boost::asio::ip::tcp::acceptor> tcp_acceptor;
        typedef io::fiberized<boost::asio::ip::tcp::socket> tcp_socket;
        typedef io::fiberized<boost::asio::ip::tcp::resolver> tcp_resolver;
    }
    using io::tcp_acceptor;
    using io::tcp_socket;
    using io::tcp_resolver;
}   // End of namespace fibio

#endif
