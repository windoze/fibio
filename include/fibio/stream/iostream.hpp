//
//  iostream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-4.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_socket_stream_hpp
#define fibio_socket_stream_hpp

#include <fibio/io/io.hpp>
#include <fibio/stream/streambuf.hpp>

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

        fiberized_iostream()
        : streambase_t()
        , std::iostream(&(this->sbuf_))
        {}
        
        ~fiberized_iostream()
        {}
        
        /**
         * Constructor
         *
         * @param s underlying stream device, such as socket or pipe
         */
        fiberized_iostream(stream_type &&s)
        : streambase_t(std::move(s))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Movable
        fiberized_iostream(fiberized_iostream &&src)
        : streambase_t(std::move(src))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Non-copyable
        fiberized_iostream(const fiberized_iostream&) = delete;
        fiberized_iostream& operator=(const fiberized_iostream&) = delete;
        
        /*
        void swap(fiberized_iostream &other) {
            this->sbuf_.swap(other.sbuf_);
        }
        
        void assign(fiberized_iostream &&other) {
            this->sbuf_.swap(other.sbuf_);
        }
         */
        
        template <typename... T>
        std::error_code open(T... x) {
            return this->sbuf_.open(x...);
        }
        
        template <typename... T>
        auto connect(T... x) -> decltype(this->sbuf_.connect(x...)) {
            return this->sbuf_.connect(x...);
        }
        
        /**
         * Close underlying stream device, flushing if necessary
         */
        inline void close() {
            if(streambuf().is_open()) {
                flush();
            }
            streambuf().close();
        }
        
        inline bool is_open() const
        { return this->sbuf_.is_open(); }
        
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
    
    typedef fiberized_iostream<tcp_socket> tcp_stream;
    typedef fiberized_iostream<posix_stream_descriptor> posix_stream;

    template<typename Stream>
    struct acceptor;
    
    template<>
    struct acceptor<tcp_stream> {
        typedef tcp_stream socket_type;
        typedef io::tcp_acceptor acceptor_type;
        typedef typename asio::ip::tcp::endpoint endpoint_type;

        acceptor(const std::string &s, unsigned short port_num)
        : acc_(endpoint_type(asio::ip::address::from_string(s.c_str()), port_num))
        {}
        
        acceptor(unsigned short port_num)
        : acc_(endpoint_type(asio::ip::address(), port_num))
        {}
        
        template<typename Rep, typename Period>
        acceptor(const std::string &s, unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(asio::ip::address::from_string(s.c_str()), port_num))
        { acc_.set_accept_timeout(timeout_duration); }

        template<typename Rep, typename Period>
        acceptor(unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(endpoint_type(asio::ip::address(), port_num))
        { acc_.set_accept_timeout(timeout_duration); }
        
        acceptor(acceptor &&other)
        : acc_(std::move(other.acc_))
        {}
        
        acceptor(const acceptor &other)=delete;
        acceptor &operator=(const acceptor &other)=delete;
        
        void close()
        { acc_.close(); }
        
        template<typename Rep, typename Period>
        void set_accept_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { acc_.set_accept_timeout(timeout_duration); }
        
        tcp_stream accept() {
            socket_type s;
            acc_.accept(s.sbuf_);
            return tcp_stream(std::move(s));
        }
        
        tcp_stream accept(std::error_code &ec) {
            socket_type s;
            acc_.accept(s.sbuf_, ec);
            return tcp_stream(std::move(s));
        }
        
        void accept(tcp_stream &s)
        { acc_.accept(s.sbuf_); }
        
        void accept(tcp_stream &s, std::error_code &ec)
        { acc_.accept(s.sbuf_, ec); }
        
        tcp_stream operator()()
        { return accept(); }
        
        tcp_stream operator()(std::error_code &ec)
        { return accept(ec); }
        
        void operator()(tcp_stream &s)
        { accept(s); }
        
        void operator()(tcp_stream &s, std::error_code &ec)
        { accept(s, ec); }
        
        acceptor_type acc_;
    };

    typedef acceptor<tcp_stream> tcp_acceptor;
}}  // End of namespace fibio::stream

namespace fibio {
    using stream::tcp_stream;
    using stream::posix_stream;
    using stream::tcp_acceptor;
}

#endif
