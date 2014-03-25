//
//  tcp.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_tcp_hpp
#define fibio_tcp_hpp

#include <asio/ip/tcp.hpp>

namespace fibio { namespace io {
    namespace tcp {
        typedef asio::ip::tcp protocol;
        typedef asio::ip::tcp::resolver::query query;
        typedef asio::ip::tcp::endpoint endpoint;
        typedef asio::ip::tcp::socket socket;
        typedef asio::ip::tcp::acceptor acceptor;
        typedef asio::ip::tcp::socket::shutdown_type shutdown_type;
    }   // End of namespace fibio::io::tcp
    
    // Resolve
    tcp::endpoint resolve(const tcp::query &addr);
    tcp::endpoint resolve(const tcp::query &addr, std::error_code &);

    // Listen
    tcp::acceptor listen(const tcp::endpoint &ep, bool reuse_addr=true);
    tcp::acceptor listen(const tcp::endpoint &ep, bool reuse_addr, std::error_code &);
    // Simplified version
    tcp::acceptor listen(const char *s, unsigned short port_num);
    tcp::acceptor listen(unsigned short port_num);

    // Accept
    tcp::socket accept(tcp::acceptor &a, uint64_t timeout_usec=0);
    tcp::socket accept(tcp::acceptor &a, uint64_t timeout_usec, std::error_code &);

    // Connect
    tcp::socket connect(const tcp::endpoint &remote_ep, const tcp::endpoint &local_ep=tcp::endpoint(), uint64_t timeout_usec=0);
    std::error_code connect(tcp::socket &,const tcp::endpoint &remote_ep, const tcp::endpoint &local_ep, uint64_t timeout_usec);
    
    // Read
    size_t read_some(tcp::socket &s, char *buffer, size_t sz, uint64_t timeout_usec=0);
    size_t read_some(tcp::socket &s, char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
    
    // Write
    size_t write_some(tcp::socket &s, const char *buffer, size_t sz, uint64_t timeout_usec=0);
    size_t write_some(tcp::socket &s, const char *buffer, size_t sz, uint64_t timeout_usec, std::error_code &);
    
    // Shutdown
    std::error_code shutdown(tcp::socket &s, tcp::shutdown_type);
}}  // End of namespace fibio::io

#endif
