//
//  local_datagram.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_local_datagram_hpp
#define fibio_local_datagram_hpp

#include <asio/local/datagram_protocol.hpp>

namespace fibio { namespace io {
    namespace local_datagram {
        typedef asio::local::datagram_protocol::endpoint endpoint;
        typedef asio::local::datagram_protocol::socket socket;
    }   // End of namespace fibio::io::local_datagram
    
    // Open and bind
    local_datagram::socket listen(const local_datagram::endpoint &ep,
                                  bool reuse_addr=true);
    // Connect to remote, optionally bind to local
    local_datagram::socket connect(const local_datagram::endpoint &remote_ep,
                                   const local_datagram::endpoint &local_ep=local_datagram::endpoint(),
                                   uint64_t timeout=0);
}}  // End of namespace fibio::io

#endif
