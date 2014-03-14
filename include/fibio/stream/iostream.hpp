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
        typedef streambuf<StreamDescriptor> streambuf_t;
        
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
    class basic_iostream : private iostream_base<StreamDescriptor>, public std::iostream {
    public:
        typedef streambuf<StreamDescriptor> streambuf_t;
        typedef iostream_base<StreamDescriptor> streambase_t;
        
        basic_iostream()
        : streambase_t()
        , std::iostream(&(this->sbuf_))
        {}
        
        /**
         * Constructor
         *
         * @param s underlying stream device, such as socket or pipe
         */
        basic_iostream(StreamDescriptor &&s)
        : streambase_t(std::move(s))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Non-movable
        basic_iostream(basic_iostream &&src)
        : streambase_t(std::move(src))
        , std::iostream(&(this->sbuf_))
        {}
        
        // Non-copyable
        basic_iostream(const basic_iostream&) = delete;
        basic_iostream& operator=(const basic_iostream&) = delete;
        
        void swap(basic_iostream &other) {
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
    };

    typedef basic_iostream<fibio::io::tcp::socket> tcp_stream;
    typedef basic_iostream<fibio::io::posix::stream_descriptor> posix_stream;
}}  // End of namespace fibio::stream

namespace std {
    template<typename StreamDescriptor>
    void swap(fibio::stream::basic_iostream<StreamDescriptor> &lhs, fibio::stream::basic_iostream<StreamDescriptor> &rhs) {
        lhs.swap(rhs);
    }
}

#endif
