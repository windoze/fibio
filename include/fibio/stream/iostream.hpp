//
//  iostream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_iostream_hpp
#define fibio_stream_iostream_hpp

#include <map>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/lexical_cast.hpp>
#include <fibio/fiber.hpp>
#include <fibio/future.hpp>
#include <fibio/stream/streambuf.hpp>

namespace fibio { namespace stream {
    namespace detail {
        template<typename Stream>
        struct iostream_base {
            typedef fiberized_streambuf<Stream> streambuf_t;
            iostream_base()=default;
            iostream_base(iostream_base &&other)
            : sbuf_(std::move(other.sbuf_))
            {}
            iostream_base(streambuf_t &&other_buf)
            : sbuf_(std::move(other_buf))
            {}
            // For SSL stream, construct with ssl::context
            template<typename Arg>
            iostream_base(Arg &arg)
            : sbuf_(arg)
            {}
            streambuf_t sbuf_;
        };
    }
    
    // Closable stream
    class fiberized_iostream_base : public std::iostream {
    public:
        typedef std::iostream base_type;
        using base_type::base_type;
        virtual bool is_open() const=0;
        virtual void close()=0;
    };
    
    template<typename Stream>
    class fiberized_iostream
    : public detail::iostream_base<Stream>
    , public fiberized_iostream_base
    {
        typedef fiberized_streambuf<Stream> streambuf_t;
        typedef detail::iostream_base<Stream> streambase_t;
    public:
        typedef typename streambuf_t::stream_type stream_type;
        typedef typename stream_type::lowest_layer_type::protocol_type protocol_type;

        fiberized_iostream()
        : streambase_t()
        , fiberized_iostream_base(&(this->sbuf_))
        {}
        
        // For SSL stream, construct with ssl::context
        template<typename Arg>
        fiberized_iostream(Arg &arg)
        : streambase_t(arg)
        , fiberized_iostream_base(&(this->sbuf_))
        {}
        
        // Movable
        fiberized_iostream(fiberized_iostream &&src)//=default;
        : streambase_t(std::move(src))
        , fiberized_iostream_base(&(this->sbuf_))
        {}
        
        // Non-copyable
        fiberized_iostream(const fiberized_iostream&) = delete;
        fiberized_iostream& operator=(const fiberized_iostream&) = delete;
        
        template <typename... T>
        boost::system::error_code open(T... x) {
            return this->sbuf_.lowest_layer().open(x...);
        }

        template <typename Arg>
        auto connect(const Arg &arg) -> decltype(this->sbuf_.connect(arg)) {
            return this->sbuf_.connect(arg);
        }
        
        boost::system::error_code connect(const char *host, const char *service) {
            return connect(std::string(host), std::string(service));
        }
        
        boost::system::error_code connect(const std::string &host, const std::string &service) {
            boost::system::error_code ec;
            typedef typename protocol_type::resolver resolver_type;
            typedef typename resolver_type::query query_type;
            resolver_type r(asio::get_io_service());
            query_type q(host, service);
            typename resolver_type::iterator i=r.async_resolve(q, asio::yield[ec]);
            if (ec) return ec;
            return this->sbuf_.connect(i->endpoint());
        }
        
        /**
         * Close underlying stream device, flushing if necessary
         */
        inline void close() {
            if(streambuf().lowest_layer().is_open()) {
                flush();
            }
            streambuf().lowest_layer().close();
        }
        
        inline bool is_open() const
        { return this->sbuf_.lowest_layer().is_open(); }
        
        inline streambuf_t &streambuf()
        { return this->sbuf_; }
        
        inline const streambuf_t &streambuf() const
        { return this->sbuf_; }
        
        inline stream_type &stream_descriptor()
        { return streambuf(); }
        
        void set_duplex_mode(duplex_mode dm)
        { this->sbuf_.set_duplex_mode(dm); }
        
        duplex_mode get_duplex_mode() const
        { return this->sbuf_.get_duplex_mode(); }
    };
    
    template<typename Stream>
    fiberized_iostream<Stream> &operator<<(fiberized_iostream<Stream> &is, duplex_mode dm) {
        is.set_duplex_mode(dm);
        return is;
    }

    // streams
    typedef fiberized_iostream<boost::asio::ip::tcp::socket> tcp_stream;
    typedef fiberized_iostream<boost::asio::posix::stream_descriptor> posix_stream;
    typedef fiberized_iostream<boost::asio::local::stream_protocol::socket> local_stream;
    
    template<typename Stream>
    struct stream_acceptor {
        typedef Stream stream_type;
        typedef typename Stream::stream_type socket_type;
        typedef typename stream_type::protocol_type::acceptor acceptor_type;
        typedef typename stream_type::protocol_type::endpoint endpoint_type;

        stream_acceptor(const std::string &s, unsigned short port_num)
        : acc_(asio::get_io_service(),
               endpoint_type(boost::asio::ip::address::from_string(s.c_str()), port_num))
        {}
        
        stream_acceptor(unsigned short port_num)
        : acc_(asio::get_io_service(),
               endpoint_type(boost::asio::ip::address(), port_num))
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
        
        stream_type accept() {
            stream_type s;
            boost::system::error_code ec;
            accept(s, ec);
            return s;
        }
        
        stream_type accept(boost::system::error_code &ec) {
            stream_type s;
            accept(s, ec);
            return s;
        }
        
        boost::system::error_code accept(stream_type &s) {
            boost::system::error_code ec;
            accept(s, ec);
            return ec;
        }
        
        void accept(stream_type &s, boost::system::error_code &ec)
        { acc_.async_accept(s.streambuf(), asio::yield[ec]); }
        
        stream_type operator()()
        { return accept(); }
        
        stream_type operator()(boost::system::error_code &ec)
        { return accept(ec); }
        
        boost::system::error_code operator()(stream_type &s)
        { return accept(s); }
        
        void operator()(stream_type &s, boost::system::error_code &ec)
        { accept(s, ec); }
        
        acceptor_type acc_;
    };
    
    // acceptors
    typedef stream_acceptor<tcp_stream> tcp_stream_acceptor;
    typedef stream_acceptor<local_stream> local_stream_acceptor;
    
    template<typename Stream>
    struct stream_traits {
        typedef Stream stream_type;
        typedef typename stream_type::stream_type socket_type;
        typedef stream_acceptor<stream_type> acceptor_type;
        typedef typename acceptor_type::endpoint_type endpoint_type;
        // HACK:
        typedef int arg_type;
        static std::unique_ptr<stream_type> construct(arg_type *arg) {
            return std::unique_ptr<stream_type>(new stream_type());
        }
    };
    
    template<typename R>
    R make_endpoint(const std::string &access_point) {}
    
    template<>
    inline boost::asio::ip::tcp::endpoint make_endpoint<boost::asio::ip::tcp::endpoint>(const std::string &access_point) {
        auto i=access_point.find(':');
        // TODO: IPv4 and IPv6
        std::string addr("0::0");
        std::string port;
        if(i==access_point.npos) {
            // Assume arg contains only port
            port=access_point;
        } else {
            addr.assign(access_point.begin(), access_point.begin()+i);
            port.assign(access_point.begin()+i+1, access_point.end());
        }
        boost::system::error_code ec;
        boost::asio::ip::address a=boost::asio::ip::address::from_string(addr, ec);
        if (ec) {
            // Address is *not* an IP address
            boost::asio::ip::tcp::resolver r(asio::get_io_service());
            boost::asio::ip::tcp::resolver::query q(addr, port);
            boost::asio::ip::tcp::resolver::iterator i=r.async_resolve(q, asio::yield[ec]);
            if(ec) {
                BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "Resolving address failed"));
            }
            return i->endpoint();
        } else {
            return boost::asio::ip::tcp::endpoint(a, boost::lexical_cast<uint16_t>(port));
        }
    }
    
    template<>
    inline boost::asio::local::stream_protocol::endpoint make_endpoint<boost::asio::local::stream_protocol::endpoint>(const std::string &access_point) {
        return boost::asio::local::stream_protocol::endpoint(access_point);
    }
    
    template<typename Stream>
    struct listener {
        typedef Stream stream_type;
        typedef stream_traits<Stream> traits_type;
        typedef typename traits_type::acceptor_type acceptor_type;
        typedef typename traits_type::endpoint_type endpoint_type;
        typedef typename traits_type::arg_type arg_type;

        template<typename std::enable_if<std::is_move_constructible<Stream>::value>::type* = nullptr >
        listener(const std::string &access_point)
        : ep_(make_endpoint<endpoint_type>(access_point))
        {}
        
        // For SSL, arg type is ssl::context
        template<typename Arg>
        listener(Arg &arg, const std::string &access_point)
        : arg_(&arg)
        , ep_(make_endpoint<endpoint_type>(access_point))
        {}
        
        endpoint_type endpoint() const {
            return ep_;
        }
        
        // Start and join, other fiber may stop the listener
        template<typename F>
        void operator()(F f) {
            start(f);
            join();
        }
        
        // Start listener
        template<typename F>
        void start(F f) {
            if(!stop_signal_) {
                stop_signal_.reset(new promise<void>);
                if(!acceptor_fiber_) {
                    acceptor_fiber_.reset(new fiber(&listener::acceptor_fiber<F>,
                                                    this,
                                                    ep_,
                                                    stop_signal_.get(),
                                                    f));
                }
            }
        }
        
        void stop() {
            if(stop_signal_) {
                stop_signal_->set_value();
            }
        }
        
        void join() {
            if (acceptor_fiber_) {
                acceptor_fiber_->join();
                acceptor_fiber_.reset();
                stop_signal_.reset();
            }
        }
        
    private:
        template<typename F>
        void acceptor_fiber(const endpoint_type & e, promise<void> *p, F f) {
            acceptor_type acc(e);
            boost::system::error_code ec;
            fiber watchdog(fiber::attributes(fiber::attributes::stick_with_parent),
                           &listener::acceptor_watchdog_fiber,
                           this,
                           p,
                           std::ref(acc));
            while(!ec) {
                std::unique_ptr<stream_type> s(traits_type::construct(arg_));
                acc(*s, ec);
                if(ec) break;
                fiber([f](std::unique_ptr<stream_type> str){
                    f(*str);
                }, std::move(s)).detach();
            }
            watchdog.join();
        }
        
        void acceptor_watchdog_fiber(promise<void> *p, acceptor_type &acc) {
            p->get_future().wait();
            acc.close();
        }
        
        arg_type *arg_=nullptr;
        endpoint_type ep_;
        std::unique_ptr<fiber> acceptor_fiber_;
        std::unique_ptr<promise<void>> stop_signal_;
    };
    
    // listeners
    typedef listener<tcp_stream> tcp_listener;
    typedef listener<local_stream> local_listener;
}}  // End of namespace fibio::stream

namespace fibio {
    using stream::tcp_stream;
    using stream::posix_stream;
    using stream::local_stream;
    using stream::tcp_stream_acceptor;
    using stream::local_stream_acceptor;
    using stream::tcp_listener;
    using stream::local_listener;
}

#endif
