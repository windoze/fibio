//
//  ssl.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-28.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_ssl_hpp
#define fibio_stream_ssl_hpp

#include <fibio/io/ssl.hpp>
#include <fibio/stream/iostream.hpp>

namespace fibio { namespace stream {
    // Acceptor for SSL over stream socket
    template<typename Stream>
    struct stream_acceptor<fiberized_iostream<io::fiberized<asio::ssl::stream<Stream>>>> {
        // Stream is the fiberized_iostream wrap over ssl stream with underlying socket is fiberized<Stream>
        typedef fiberized_iostream<io::fiberized<asio::ssl::stream<Stream>>> stream_type;
        // Socket type is io::fiberized<socket>
        typedef typename io::fiberized<Stream> socket_type;
        // Acceptor type is the acceptor work with underlying socket
        typedef typename io::fiberized<typename socket_type::protocol_type::acceptor> acceptor_type;
        // Endpoint is for underlying socket
        typedef typename Stream::protocol_type::endpoint endpoint_type;
        
        stream_acceptor(const std::string &s, unsigned short port_num)
        : acc_(endpoint_type(asio::ip::address::from_string(s.c_str()), port_num))
        {}
        
        stream_acceptor(unsigned short port_num)
        : acc_(endpoint_type(asio::ip::address(), port_num))
        {}
        
        template<typename Rep, typename Period>
        stream_acceptor(const std::string &s, unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(asio::ip::address::from_string(s.c_str()), port_num))
        { acc_.set_accept_timeout(timeout_duration); }
        
        template<typename Rep, typename Period>
        stream_acceptor(unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(asio::ip::address(), port_num))
        { acc_.set_accept_timeout(timeout_duration); }
        
        stream_acceptor(stream_acceptor &&other)
        : acc_(std::move(other.acc_))
        {}
        
        stream_acceptor(const stream_acceptor &other)=delete;
        stream_acceptor &operator=(const stream_acceptor &other)=delete;
        
        void close()
        { acc_.close(); }
        
        template<typename Rep, typename Period>
        void set_accept_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { acc_.set_accept_timeout(timeout_duration); }
        
        stream_type accept() {
            stream_type s;
            acc_.accept(s.streambuf().next_layer());
            return s;
        }
        
        stream_type accept(std::error_code &ec) {
            stream_type s;
            acc_.accept(s.streambuf().next_layer(), ec);
            return s;
        }
        
        void accept(stream_type &s)
        { acc_.accept(s.streambuf().next_layer()); }
        
        void accept(stream_type &s, std::error_code &ec)
        { acc_.accept(s.streambuf().next_layer(), ec); }
        
        stream_type operator()()
        { return accept(); }
        
        stream_type operator()(std::error_code &ec)
        { return accept(ec); }
        
        void operator()(stream_type &s)
        { accept(s); }
        
        void operator()(stream_type &s, std::error_code &ec)
        { accept(s, ec); }
        
        acceptor_type acc_;
    };
}}

namespace fibio { namespace ssl {
    typedef stream::fiberized_iostream<io::fiberized<asio::ssl::stream<asio::ip::tcp::socket>>> tcp_stream;
    typedef stream::stream_acceptor<tcp_stream> tcp_stream_acceptor;
}}

#endif
