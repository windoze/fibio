//
//  udp.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_ip_udp_hpp
#define fibio_io_ip_udp_hpp

#include <boost/asio/ip/udp.hpp>
#include <fibio/io/basic_datagram_socket.hpp>
#include <fibio/io/ip/basic_resolver.hpp>

namespace fibio {
    namespace io {
        typedef io::fiberized<boost::asio::ip::udp::socket> udp_socket;
        typedef io::fiberized<boost::asio::ip::udp::resolver> udp_resolver;
    }
    using io::udp_socket;
    using io::udp_resolver;
}   // End of namespace fibio

#endif
