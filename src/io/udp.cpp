//
//  udp.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-6.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/ip/udp.hpp>
#include "../fiber/fiber_object.hpp"
#include "io_ops.hpp"

namespace fibio { namespace io {
    //using namespace udp;
    asio::ip::udp::endpoint resolve(const asio::ip::udp::resolver::query &q) {
        return detail::resolve<asio::ip::udp>(q);
    }
    
    asio::ip::udp::endpoint resolve(const asio::ip::udp::resolver::query &q,
                                           std::error_code &ec) {
        return detail::resolve<asio::ip::udp>(q, ec);
    }
    
#if 0
    // TODO:
    socket listen(const endpoint &ep, bool reuse_addr=true) {
        
    }

    socket connect(const endpoint &remote_ep, const endpoint &local_ep=endpoint()) {
        
    }
#endif
}}  // End of namespace fibio::io
