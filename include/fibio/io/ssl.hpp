//
//  ssl.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-28.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_ssl_hpp
#define fibio_io_ssl_hpp

#include <fibio/io/ip/tcp.hpp>
#include <fibio/io/ssl/stream.hpp>

namespace fibio { namespace ssl {
    // Introduce some useful types
    using boost::asio::ssl::context;
    using boost::asio::ssl::rfc2818_verification;
    using boost::asio::ssl::verify_context;
    typedef boost::asio::ssl::stream_base::handshake_type handshake_type;
    
    typedef io::fiberized<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> tcp_socket;
}}  // End of namespace fibio::ssl

#endif
