//
//  ssl.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-28.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_ssl_hpp
#define fibio_stream_ssl_hpp

#include <boost/asio/ssl.hpp>
#include <fibio/stream/iostream.hpp>

namespace fibio { namespace stream {
    // Acceptor for SSL over stream socket
    template<typename Stream>
    struct stream_acceptor<fiberized_iostream<boost::asio::ssl::stream<Stream>>> {
        typedef fiberized_iostream<boost::asio::ssl::stream<Stream>> stream_type;
        typedef Stream socket_type;
        typedef typename socket_type::protocol_type::acceptor acceptor_type;
        typedef typename socket_type::protocol_type::endpoint endpoint_type;
        
        stream_acceptor(const std::string &s, unsigned short port_num)
        : acc_(asio::get_io_service(),
               endpoint_type(boost::asio::ip::address::from_string(s.c_str()), port_num))
        {}
        
        stream_acceptor(unsigned short port_num)
        : acc_(asio::get_io_service(),
               endpoint_type(boost::asio::ip::address(), port_num))
        {}
        
        stream_acceptor(const char *access_point)
        : stream_acceptor(detail::make_endpoint<endpoint_type>(access_point))
        {}
        
        stream_acceptor(const std::string &access_point)
        : stream_acceptor(detail::make_endpoint<endpoint_type>(access_point))
        {}
        
        stream_acceptor(const endpoint_type &ep)
        : acc_(asio::get_io_service(), ep)
        {}
        
        stream_acceptor(stream_acceptor &&other)
        : acc_(std::move(other.acc_))
        {}
        
        stream_acceptor(const stream_acceptor &other)=delete;
        stream_acceptor &operator=(const stream_acceptor &other)=delete;
        
        void close()
        { acc_.close(); }
        
        boost::system::error_code accept(stream_type &s) {
            boost::system::error_code ec;
            accept(s, ec);
            return ec;
        }
        
        void accept(stream_type &s, boost::system::error_code &ec) {
            acc_.async_accept(s.streambuf().next_layer(), asio::yield[ec]);
            if(ec) return;
            s.streambuf().async_handshake(boost::asio::ssl::stream_base::server, asio::yield[ec]);
        }
        
        boost::system::error_code operator()(stream_type &s)
        { return accept(s); }
        
        void operator()(stream_type &s, boost::system::error_code &ec)
        { accept(s, ec); }
        
        acceptor_type acc_;
    };
    
    template<typename Socket>
    struct stream_traits<stream::fiberized_iostream<boost::asio::ssl::stream<Socket>>> {
        typedef stream::fiberized_iostream<boost::asio::ssl::stream<Socket>> stream_type;
        typedef typename stream_type::stream_type socket_type;
        typedef stream_acceptor<stream_type> acceptor_type;
        typedef typename acceptor_type::endpoint_type endpoint_type;
        // HACK:
        typedef boost::asio::ssl::context arg_type;
        static std::unique_ptr<stream_type> construct(arg_type *arg) {
            return std::unique_ptr<stream_type>(new stream_type(*arg));
        }
    };
}}  // End of namespace fibio::stream

namespace fibio { namespace ssl {
    // Introduce some useful types
    using boost::asio::ssl::context;
    using boost::asio::ssl::rfc2818_verification;
    using boost::asio::ssl::verify_context;
    typedef boost::asio::ssl::stream_base::handshake_type handshake_type;
    
    typedef stream::fiberized_iostream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> tcp_stream;
    typedef stream::stream_acceptor<tcp_stream> tcp_stream_acceptor;
    typedef stream::listener<tcp_stream> tcp_listener;
}}

#endif
