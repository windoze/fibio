//
//  iostream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_iostream_hpp
#define fibio_stream_iostream_hpp

#include <fibio/io/io.hpp>
#include <fibio/stream/streambuf.hpp>
#include <fibio/io/basic_stream_socket.hpp>
#include <fibio/io/posix/stream_descriptor.hpp>
#include <fibio/io/local/stream_protocol.hpp>

namespace fibio { namespace stream {
    namespace detail {
        template<typename... Stream>
        struct iostream_base {
            typedef fiberized_streambuf<Stream...> streambuf_t;
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
    
    template<typename... Stream>
    class fiberized_iostream
    : public detail::iostream_base<Stream...>
    , public std::iostream
    {
        typedef fiberized_streambuf<Stream...> streambuf_t;
        typedef detail::iostream_base<Stream...> streambase_t;
    public:
        typedef typename streambuf_t::stream_type stream_type;
        typedef typename stream_type::lowest_layer_type::protocol_type protocol_type;

        fiberized_iostream()
        : streambase_t()
        , std::iostream(&(this->sbuf_))
        {}
        
        // For SSL stream, construct with ssl::context
        template<typename Arg>
        fiberized_iostream(Arg &arg)
        : streambase_t(arg)
        , std::iostream(&(this->sbuf_))
        {}
        
        // Movable
        fiberized_iostream(fiberized_iostream &&src)//=default;
        : streambase_t(std::move(src))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Non-copyable
        fiberized_iostream(const fiberized_iostream&) = delete;
        fiberized_iostream& operator=(const fiberized_iostream&) = delete;
        
        template <typename... T>
        boost::system::error_code open(T... x) {
            return this->sbuf_.lowest_layer().open(x...);
        }
        
        template <typename... T>
        auto connect(T... x) -> decltype(this->sbuf_.connect(x...)) {
            return this->sbuf_.connect(x...);
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
        
        template<typename Rep, typename Period>
        void set_connect_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { this->sbuf_.set_connect_timeout(timeout_duration); }
        
        template<typename Rep, typename Period>
        void set_read_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { this->sbuf_.set_read_timeout(timeout_duration); }
        
        template<typename Rep, typename Period>
        void set_write_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { this->sbuf_.set_write_timeout(timeout_duration); }
        
        void set_duplex_mode(duplex_mode dm)
        { this->sbuf_.set_duplex_mode(dm); }
        
        duplex_mode get_duplex_mode() const
        { return this->sbuf_.get_duplex_mode(); }
    };
    
    template<typename... Stream>
    fiberized_iostream<Stream...> &operator<<(fiberized_iostream<Stream...> &is, duplex_mode dm) {
        is.set_duplex_mode(dm);
        return is;
    }

    template<typename Stream>
    struct stream_acceptor {
        typedef typename Stream::stream_type socket_type;
        typedef typename io::fiberized<typename Stream::protocol_type::acceptor> acceptor_type;
        typedef typename Stream::protocol_type::endpoint endpoint_type;

        stream_acceptor(const std::string &s, unsigned short port_num)
        : acc_(endpoint_type(boost::asio::ip::address::from_string(s.c_str()), port_num))
        {}
        
        stream_acceptor(unsigned short port_num)
        : acc_(endpoint_type(boost::asio::ip::address(), port_num))
        {}
        
        template<typename Rep, typename Period>
        stream_acceptor(const std::string &s, unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(boost::asio::ip::address::from_string(s.c_str()), port_num))
        { acc_.set_accept_timeout(timeout_duration); }

        template<typename Rep, typename Period>
        stream_acceptor(unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(boost::asio::ip::address(), port_num))
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
        
        Stream accept() {
            Stream s;
            acc_.accept(s.streambuf());
            return s;
        }
        
        Stream accept(boost::system::error_code &ec) {
            Stream s;
            acc_.accept(s.streambuf(), ec);
            return s;
        }
        
        void accept(Stream &s)
        { acc_.accept(s.streambuf()); }
        
        void accept(Stream &s, boost::system::error_code &ec)
        { acc_.accept(s.streambuf(), ec); }
        
        Stream operator()()
        { return accept(); }
        
        Stream operator()(boost::system::error_code &ec)
        { return accept(ec); }
        
        void operator()(Stream &s)
        { accept(s); }
        
        void operator()(Stream &s, boost::system::error_code &ec)
        { accept(s, ec); }
        
        acceptor_type acc_;
    };
    
    // streams
    typedef fiberized_iostream<tcp_socket> tcp_stream;
    typedef fiberized_iostream<posix_stream_descriptor> posix_stream;
    typedef fiberized_iostream<local_stream_socket> local_stream;

    // acceptors
    typedef stream_acceptor<tcp_stream> tcp_stream_acceptor;
    typedef stream_acceptor<local_stream> local_stream_acceptor;
}}  // End of namespace fibio::stream

namespace fibio {
    using stream::tcp_stream;
    using stream::posix_stream;
    using stream::local_stream;
    using stream::tcp_stream_acceptor;
    using stream::local_stream_acceptor;
}

#endif
