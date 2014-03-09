//
//  local_stream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_local_stream_hpp
#define fibio_local_stream_hpp

#include <asio/local/stream_protocol.hpp>

namespace fibio { namespace io {
    namespace local_stream {
        typedef asio::local::stream_protocol::endpoint endpoint;
        typedef asio::local::stream_protocol::socket socket;
        typedef asio::local::stream_protocol::acceptor acceptor;
    }   // End of namespace fibio::io::local_stream
    
    local_stream::acceptor listen(const local_stream::endpoint &ep, bool reuse_addr=true);
    local_stream::socket accept(local_stream::acceptor &a);
    local_stream::socket connect(const local_stream::endpoint &remote_ep, const local_stream::endpoint &local_ep=local_stream::endpoint(), uint64_t timeout=0);
}}  // End of namespace fibio::io

#endif
