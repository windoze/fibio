//
//  tcp.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-5.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <asio/ip/tcp.hpp>
#include "io_ops.hpp"

namespace fibio { namespace io {
    
    // Throw system_error(error_code)
    
    asio::ip::tcp::endpoint resolve(const asio::ip::tcp::resolver::query &q) {
        return detail::resolve<asio::ip::tcp>(q);
    }

    asio::ip::tcp::socket connect(const asio::ip::tcp::endpoint &remote_ep,
                                         const asio::ip::tcp::endpoint &local_ep,
                                         uint64_t timeout) {
        return detail::connect<asio::ip::tcp>(remote_ep, local_ep, timeout);
    }
    
    asio::ip::tcp::acceptor listen(const asio::ip::tcp::endpoint &ep, bool reuse_addr) {
        asio::ip::tcp::acceptor a(fibers::this_fiber::detail::get_io_service(), ep, reuse_addr);
        return a;
    }
    
    asio::ip::tcp::socket accept(asio::ip::tcp::acceptor &a) {
        return detail::accept<asio::ip::tcp>(a);
    }

    size_t read_some(asio::ip::tcp::socket &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout) {
        return detail::read_some<asio::ip::tcp::socket>(s, buffer, sz, timeout);
    }
    
    size_t write_some(asio::ip::tcp::socket &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout) {
        return detail::write_some<asio::ip::tcp::socket>(s, buffer, sz, timeout);
    }
    
    // Return error_code
    
    asio::ip::tcp::endpoint resolve(const asio::ip::tcp::resolver::query &q, std::error_code &ec) {
        return detail::resolve<asio::ip::tcp>(q, ec);
    }
    
    std::error_code connect(asio::ip::tcp::socket &s,
                                      const asio::ip::tcp::endpoint &remote_ep,
                                      const asio::ip::tcp::endpoint &local_ep,
                                      uint64_t timeout) {
        return detail::connect<asio::ip::tcp>(s, remote_ep, local_ep, timeout);
    }
    
    asio::ip::tcp::socket accept(asio::ip::tcp::acceptor &a,
                                        std::error_code &ec) {
        return detail::accept<asio::ip::tcp>(a, ec);
    }
    
    size_t read_some(asio::ip::tcp::socket &s,
                     char *buffer,
                     size_t sz,
                     uint64_t timeout,
                     std::error_code &ec) {
        return detail::read_some<asio::ip::tcp::socket>(s, buffer, sz, timeout, ec);
    }
    
    size_t write_some(asio::ip::tcp::socket &s,
                      const char *buffer,
                      size_t sz,
                      uint64_t timeout,
                      std::error_code &ec) {
        return detail::write_some<asio::ip::tcp::socket>(s, buffer, sz, timeout, ec);
    }
}}   // End of namespace fibio::io
