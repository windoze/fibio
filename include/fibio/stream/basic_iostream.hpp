//
//  basic_iostream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_basic_iostream_hpp
#define fibio_basic_iostream_hpp

#include <streambuf>
#include <iostream>
#include <chrono>
#include <fibio/fibers/fiber.hpp>

namespace fibio { namespace stream {
    constexpr size_t bf_size=4096;
    constexpr std::streamsize pb_size=4;
    
    template<typename StreamDescriptor>
    class streambuf : public std::streambuf {
    public:
        streambuf()
        : sd_(fibio::fibers::this_fiber::detail::get_io_service())
        {
            memset(buffer_in_, 0, bf_size);
            memset(buffer_out_, 0, bf_size);
            setg(buffer_in_ + pb_size,
                 buffer_in_ + pb_size,
                 buffer_in_ + pb_size);
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1 );
        }
        
        /**
         * Constructor
         *
         * @param sd underlying stream device, such as socket or pipe
         */
        streambuf(StreamDescriptor &&sd)
        : sd_(std::move(sd))
        {
            memset(buffer_in_, 0, bf_size);
            memset(buffer_out_, 0, bf_size);
            setg(buffer_in_ + pb_size,
                 buffer_in_ + pb_size,
                 buffer_in_ + pb_size);
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1 );
        }
        
        // Movable
        streambuf(streambuf &&src)
        : sd_(std::move(src.sd_))
        {
            memcpy(buffer_in_, src.buffer_in_, bf_size);
            memcpy(buffer_out_, src.buffer_out_, bf_size);
        }
        
        // Non-copyable
        streambuf(const streambuf&) = delete;
        streambuf& operator=(const streambuf&) = delete;
        
        ~streambuf()
        { close(); }
        
        template <typename... T>
        std::error_code open(T... x) {
            return fibio::io::open(sd_, x...);
        }

        template <typename... T>
        std::error_code connect(T... x) {
            sd_.close();
            typename StreamDescriptor::protocol_type::resolver::query q(x...);
            typename StreamDescriptor::protocol_type::endpoint ep=fibio::io::resolve(q);
            typedef typename StreamDescriptor::protocol_type::endpoint endpoint_t;
            return fibio::io::connect(sd_, ep, endpoint_t(), connect_timeout_);
        }
        
        inline bool is_open() const
        { return sd_.is_open(); }
        
        inline void close() {
            if(sd_.is_open()) {
                std::error_code ec;
                sd_.close(ec);
            }
        }
        
        typename StreamDescriptor::native_handle_type release()
        { return sd_.release(); }
        
        inline StreamDescriptor &stream_descriptor()
        { return sd_; }
        
        template<typename Rep, typename Period>
        void set_connect_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { connect_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        template<typename Rep, typename Period>
        void set_read_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { read_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        template<typename Rep, typename Period>
        void set_write_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { write_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
    protected:
        virtual int_type overflow(int_type c) override {
            if (unbuffered_) {
                if (traits_type::eq_int_type(c, traits_type::eof())) {
                    // Nothing to do.
                    return traits_type::not_eof(c);
                }
                std::error_code ec;
                async_write_char_with_timeout(c, ec);
                if(ec)
                    return traits_type::eof();
                return c;
            } else {
                if ( c != traits_type::eof() ) {
                    *pptr() = c;
                    pbump(1);
                    if (nudge_())
                        return c;
                } else {
                    c = 0;
                }
                
                return traits_type::eof();
            }
        }
        
        virtual int_type underflow() override {
            if ( gptr() < egptr() )
                return traits_type::to_int_type( *gptr() );
            
            if ( 0 > fetch_() )
                return traits_type::eof();
            else
                return traits_type::to_int_type( *gptr() );
        }
        
        virtual int sync() override
        { return nudge_()==0 ? 0 : -1; }
        
        virtual std::streamsize showmanyc() override {
            if ( gptr() == egptr() ) {
                // Getting area is empty
                return fetch_();
            }
            return egptr()-gptr();
        }
        
        virtual std::streambuf* setbuf(char_type* s, std::streamsize n) override {
            if (pptr() == pbase() && s == 0 && n == 0) {
                // Turn off output buffering
                unbuffered_ = true;
                setp(0, 0);
                return this;
            }
            return 0;
        }
        
    private:
        void async_write_char_with_timeout(char c, std::error_code &ec) {
            fibio::io::write_some(sd_, &c, 1, write_timeout_, ec);
        }
        
        size_t async_read_some_with_timeout(std::error_code &ec) {
            return fibio::io::read_some(sd_,
                                        buffer_in_ + pb_size,
                                        bf_size - pb_size,
                                        read_timeout_,
                                        ec);
        }
        
        void async_write_with_timeout(std::error_code &ec) {
            char *ptr=pbase();
            size_t size=pptr()-pbase();
            while(size>0) {
                size_t bytes_transferred=fibio::io::write_some(sd_,
                                                               ptr,
                                                               size,
                                                               write_timeout_,
                                                               ec);
                ptr+=bytes_transferred;
                size-=bytes_transferred;
            }
        }
        
        int_type fetch_() {
            std::streamsize num = std::min(static_cast<std::streamsize>(gptr() - eback()),
                                           pb_size);
            
            std::memmove(buffer_in_ + (pb_size - num),
                         gptr() - num,
                         num);
            
            std::error_code ec;
            std::size_t n = async_read_some_with_timeout(ec);
            if (ec) {
                setg(0, 0, 0);
                return -1;
            }
            setg(buffer_in_ + pb_size - num,
                 buffer_in_ + pb_size,
                 buffer_in_ + pb_size + n);
            return n;
        }
        
        int_type nudge_() {
            // Don't flush empty buffer
            if(pptr()<=pbase()) return 0;
            std::error_code ec;
            async_write_with_timeout(ec);
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1);
            return ec ? traits_type::eof() : 0;
        }
        
        StreamDescriptor sd_;
        char buffer_in_[bf_size];
        char buffer_out_[bf_size];
        int connect_timeout_=0;
        int read_timeout_=0;
        int write_timeout_=0;
        bool unbuffered_=false;
    };
    
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
        basic_iostream(basic_iostream &&src)=delete;
        
        // Non-copyable
        basic_iostream(const basic_iostream&) = delete;
        basic_iostream& operator=(const basic_iostream&) = delete;
        
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
}}  // End of namespace fibio::stream

#endif
