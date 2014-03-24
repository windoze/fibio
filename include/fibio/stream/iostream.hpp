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
#include <fibio/stream/basic_streambuf.hpp>

namespace fibio { namespace stream {
    template<typename StreamDescriptor>
    struct iostream_base {
        typedef basic_fibio_streambuf<StreamDescriptor> streambuf_t;
        
        iostream_base()=default;
        
        iostream_base(StreamDescriptor &&s)
        : sbuf_(std::move(s))
        {}
        
        iostream_base(iostream_base<StreamDescriptor> &&other)
        : sbuf_(std::move(other.sbuf_))
        {}
        
        streambuf_t sbuf_;
    };
    
    template<typename StreamDescriptor>
    class basic_fibio_iostream : private iostream_base<StreamDescriptor>, public std::iostream {
    public:
        typedef basic_fibio_streambuf<StreamDescriptor> streambuf_t;
        typedef iostream_base<StreamDescriptor> streambase_t;
        
        basic_fibio_iostream()
        : streambase_t()
        , std::iostream(&(this->sbuf_))
        {}
        
        ~basic_fibio_iostream()
        {}
        
        /**
         * Constructor
         *
         * @param s underlying stream device, such as socket or pipe
         */
        basic_fibio_iostream(StreamDescriptor &&s)
        : streambase_t(std::move(s))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Movable
        basic_fibio_iostream(basic_fibio_iostream &&src)
        : streambase_t(std::move(src))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Non-copyable
        basic_fibio_iostream(const basic_fibio_iostream&) = delete;
        basic_fibio_iostream& operator=(const basic_fibio_iostream&) = delete;
        
        void swap(basic_fibio_iostream &other) {
            this->sbuf_.swap(other.sbuf_);
        }
        
        template <typename... T>
        std::error_code open(T... x) {
            return this->sbuf_.open(x...);
        }
        
        template <typename... T>
        std::error_code connect(T... x) {
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
        
        inline StreamDescriptor &stream_descriptor()
        { return streambuf().stream_descriptor(); }
        
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
    
    template<typename StreamDescriptor>
    basic_fibio_iostream<StreamDescriptor> &operator<<(basic_fibio_iostream<StreamDescriptor> &is, duplex_mode dm) {
        is.set_duplex_mode(dm);
        return is;
    }
    
    typedef basic_fibio_iostream<fibio::io::tcp::socket> tcp_stream;
    typedef basic_fibio_iostream<fibio::io::posix::stream_descriptor> posix_stream;

    template<typename Stream>
    struct acceptor;
    
    template<>
    struct acceptor<tcp_stream> {
        typedef typename asio::ip::tcp::acceptor acceptor_type;
        typedef typename asio::ip::tcp::endpoint endpoint_type;

        acceptor(const char *s, unsigned short port_num)
        : acc_(io::listen(s, port_num))
        {}
        
        acceptor(unsigned short port_num)
        : acc_(io::listen(port_num))
        {}
        
        template<typename Rep, typename Period>
        acceptor(const char *s, unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(io::listen(s, port_num))
        , accept_timeout_(std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count())
        {}

        template<typename Rep, typename Period>
        acceptor(unsigned short port_num, const std::chrono::duration<Rep, Period>& timeout_duration)
        : acc_(io::listen(port_num))
        , accept_timeout_(std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count())
        {}
        
        acceptor(acceptor &&other)
        : acc_(std::move(other.acc_))
        {}
        
        acceptor(const acceptor &other)=delete;
        acceptor &operator=(const acceptor &other)=delete;
        
        template<typename Rep, typename Period>
        void set_accept_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { accept_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        tcp_stream accept()
        { return tcp_stream(io::accept(acc_, accept_timeout_)); }
        
        tcp_stream accept(std::error_code &ec)
        { return tcp_stream(io::accept(acc_, accept_timeout_, ec)); }
        
        tcp_stream operator()()
        { return accept(); }
        
        tcp_stream operator()(std::error_code &ec)
        { return accept(ec); }
        
        uint64_t accept_timeout_=0;
        acceptor_type acc_;
    };

    typedef acceptor<tcp_stream> tcp_acceptor;
}}  // End of namespace fibio::stream

namespace std {
    template<typename StreamDescriptor>
    void swap(fibio::stream::basic_fibio_iostream<StreamDescriptor> &lhs, fibio::stream::basic_fibio_iostream<StreamDescriptor> &rhs) {
        lhs.swap(rhs);
    }
}

namespace fibio {
    using stream::tcp_stream;
    using stream::posix_stream;
}

#endif
