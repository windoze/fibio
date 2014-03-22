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
        typedef asio::ip::udp protocol;
        typedef asio::ip::udp::resolver::query query;
        typedef asio::ip::udp::endpoint endpoint;
        typedef asio::ip::udp::socket socket;
    }   // End of namespace fibio::io::udp
    
    // Resolve
    udp::endpoint resolve(const udp::query &addr);
    udp::endpoint resolve(const udp::query &addr, std::error_code &) /*noexcept*/;
    
    // Bind
    std::error_code bind(udp::socket &s, const udp::endpoint &ep);
    
    // Connect to remote
    udp::socket connect(const udp::endpoint &remote_ep, uint64_t timeout=0);
    std::error_code connect(udp::socket &s, const udp::endpoint &remote_ep, uint64_t timeout=0) /*noexcept*/;
    
    // Send
    size_t send(tcp::socket &s, const char *buffer, size_t sz, uint64_t timeout_usec=0);
    size_t send(tcp::socket &s, const char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
    
    // Receive
    size_t recv(tcp::socket &s, char *buffer, size_t sz, uint64_t timeout_usec=0);
    size_t recv(tcp::socket &s, char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
    
    // Send to
    size_t send_to(tcp::socket &s, const char *buffer, size_t sz, const udp::endpoint &remote_ep, uint64_t timeout_usec=0);
    size_t send_to(tcp::socket &s, const char *buffer, size_t sz, const udp::endpoint &remote_ep, uint64_t timeout_usec, std::error_code &);
    
    // Receive from
    size_t recv_from(tcp::socket &s, char *buffer, size_t sz, const udp::endpoint &remote_ep, uint64_t timeout_usec=0);
    size_t recv_from(tcp::socket &s, char *buffer, size_t sz, const udp::endpoint &remote_ep, uint64_t timeout_usec, std::error_code &);
    
}}  // End of namespace fibio::io

#endif
