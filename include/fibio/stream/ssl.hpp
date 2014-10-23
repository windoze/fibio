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
    
    template<typename Stream>
    struct listener<fiberized_iostream<boost::asio::ssl::stream<Stream>>> {
        typedef stream_acceptor<stream::fiberized_iostream<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>> acceptor_type;
        typedef fiberized_iostream<boost::asio::ssl::stream<Stream>> stream_type;
        
        listener(boost::asio::ssl::context &ctx, const std::string &addr, const std::string &port)
        : ctx_(ctx)
        {
            add_endpoint(addr, port);
        }
        
        listener(boost::asio::ssl::context &ctx, const std::string &addr, uint16_t port)
        : ctx_(ctx)
        {
            add_endpoint(addr, boost::lexical_cast<std::string>(port));
        }
        
        listener(boost::asio::ssl::context &ctx, const std::string &addr_port)
        : ctx_(ctx)
        {
            add_endpoint(addr_port);
        }
        
        listener(boost::asio::ssl::context &ctx, uint16_t port)
        : ctx_(ctx)
        {
            add_one_endpoint("0::0", port);
            add_one_endpoint("0.0.0.0", port);
        }
        
        template<typename F>
        void operator()(F f) {
            for(auto &i : endpoints_) {
                i.second.first = new fiber(&listener::acceptor_fiber<F>,
                                           this,
                                           std::cref(i.first),
                                           &(i.second.second),
                                           f);
            }
            for(auto &i : endpoints_) {
                i.second.first->join();
                delete i.second.first;
            }
            endpoints_.clear();
        }
        
        void close() {
            for(auto &i : endpoints_) {
                i.second.second.set_value();
            }
        }
        
    private:
        typedef std::pair<std::string, uint16_t> endpoint_type;
        typedef std::pair<fiber *, promise<void>> handler_type;
        typedef std::map<endpoint_type, handler_type> endpoint_map;
        
        void add_one_endpoint(const std::string &addr, uint16_t port) {
            auto i=endpoints_.find(endpoint_type{addr, port});
            if (i==endpoints_.end()) {
                endpoints_.emplace(std::pair<endpoint_type, handler_type>{
                    endpoint_type{addr, port},
                    handler_type{nullptr, promise<void>()}
                });
            }
        }
        
        void add_endpoint(const std::string &addr, const std::string &port) {
            // Check if addr is an IP address
            boost::system::error_code ec;
            boost::asio::ip::address::from_string(addr, ec);
            if(!ec) {
                // This is an IP address
                add_one_endpoint(addr, boost::lexical_cast<uint16_t>(port));
            } else {
                // This is a host name, need resolving
                boost::asio::ip::tcp::resolver r(asio::get_io_service());
                boost::asio::ip::tcp::resolver::query q(addr, port);
                boost::system::error_code ec;
                boost::asio::ip::tcp::resolver::iterator i=r.async_resolve(q, asio::yield[ec]);
                if (ec) {
                    // TODO: Error
                    return;
                }
                // Hostname may resolve to multiple addresses
                while(i!=boost::asio::ip::tcp::resolver::iterator()) {
                    add_one_endpoint(i->endpoint().address().to_string(),
                                     i->endpoint().port());
                    ++i;
                }
            }
        }
        
        void add_endpoint(const std::string &addr_port) {
            auto i=addr_port.find(':');
            if(i==addr_port.npos) {
                // Assume arg contains only port
                // TODO: IPv4 and IPv6
                add_one_endpoint("0::0", boost::lexical_cast<uint16_t>(addr_port));
                //add_one_endpoint("0.0.0.0", boost::lexical_cast<uint16_t>(addr_port));
            } else {
                add_endpoint(std::string(addr_port.begin(), addr_port.begin()+i),
                             std::string(addr_port.begin()+i+1, addr_port.end()));
            }
        }
        
        template<typename F>
        void acceptor_fiber(const endpoint_type & e, promise<void> *p, F f) {
            acceptor_type acc(e.first, e.second);
            boost::system::error_code ec;
            fiber watchdog(fiber::attributes(fiber::attributes::stick_with_parent),
                           &listener::acceptor_watchdog_fiber,
                           this,
                           p,
                           std::ref(acc));
            while(!ec) {
                // ssl::stream is not move-constructable
                boost::asio::ssl::context *ctx=&ctx_;
                std::unique_ptr<stream_type> s(new stream_type(ctx_));
                acc(*s, ec);
                fiber([ctx, f](std::unique_ptr<stream_type> s){
                    f(*s);
                }, std::move(s)).detach();
            }
            watchdog.join();
        }
        
        void acceptor_watchdog_fiber(promise<void> *p, acceptor_type &acc) {
            p->get_future().wait();
            acc.close();
        }
        
        endpoint_map endpoints_;
        boost::asio::ssl::context &ctx_;
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
