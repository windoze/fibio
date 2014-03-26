//
//  icmp.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_icmp_hpp
#define fibio_icmp_hpp

#include <asio/ip/icmp.hpp>
#include <fibio/io/basic_raw_socket.hpp>
#include <fibio/io/ip/basic_resolver.hpp>

namespace fibio {
    namespace io {
        typedef io::fiberized<asio::ip::icmp::socket> icmp_socket;
        typedef io::fiberized<asio::ip::icmp::resolver> icmp_resolver;
    }
    using io::icmp_socket;
    using io::icmp_resolver;
}   // End of namespace fibio

#endif
