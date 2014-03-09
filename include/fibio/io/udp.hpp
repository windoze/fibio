//
//  udp.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_udp_hpp
#define fibio_udp_hpp

#include <asio/ip/udp.hpp>

namespace fibio { namespace io {
    namespace udp {
        typedef asio::ip::udp::resolver::query query;
        typedef asio::ip::udp::endpoint endpoint;
        typedef asio::ip::udp::socket socket;
    }   // End of namespace fibio::io::udp
    
    udp::endpoint resolve(const udp::query &addr);
    // Open and bind
    udp::socket listen(const udp::endpoint &ep, bool reuse_addr=true);
    // Connect to remote, optionally bind to local
    udp::socket connect(const udp::endpoint &remote_ep, const udp::endpoint &local_ep=udp::endpoint(), uint64_t timeout=0);
}}  // End of namespace fibio::io

#endif
