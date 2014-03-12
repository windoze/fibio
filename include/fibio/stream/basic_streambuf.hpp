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
    template<typename StreamDescriptor>
    class basic_streambuf : public std::streambuf {
    public:
        basic_streambuf()
        : sd_(fibio::fibers::this_fiber::detail::get_io_service())
        { init_buffers(); }
        
        basic_streambuf(StreamDescriptor &&sd)
        : sd_(std::move(sd))
        { init_buffers(); }
        
        basic_streambuf(asio::io_service &iosvc)
        : sd_(iosvc)
        { init_buffers(); }
        
        /// Destructor flushes buffered data.
        ~basic_streambuf() {
            if (pptr() != pbase())
                overflow(traits_type::eof());
        }
        
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
        virtual std::streamsize showmanyc() override {
            if ( gptr() == egptr() ) {
                underflow();
            }
            return egptr()-gptr();
        }
        
        int_type underflow() {
            if (gptr() == egptr()) {
                std::error_code ec;
                size_t bytes_transferred=io::read_some(sd_,
                                                        &get_buffer_[0]+ putback_max,
                                                        buffer_size-putback_max,
                                                        read_timeout_,
                                                        ec);
                if (ec || bytes_transferred==0) {
                    return traits_type::eof();
                }
                setg(&get_buffer_[0],
                     &get_buffer_[0] + putback_max,
                     &get_buffer_[0] + putback_max + bytes_transferred);
                return traits_type::to_int_type(*gptr());
            } else {
                return traits_type::eof();
            }
        }
        
        int_type overflow(int_type c) {
            std::error_code ec;
            if (unbuffered_) {
                if (traits_type::eq_int_type(c, traits_type::eof())) {
                    // Nothing to do.
                    return traits_type::not_eof(c);
                } else {
                    char c_=c;
                    fibio::io::write_some(sd_, &c_, 1, write_timeout_, ec);
                    if (ec)
                        return traits_type::eof();
                    return c;
                }
            } else {
                char *ptr=pbase();
                size_t size=pptr() - pbase();
                while (size > 0)
                {
                    size_t bytes_transferred=fibio::io::write_some(sd_,
                                                                   ptr,
                                                                   size,
                                                                   write_timeout_,
                                                                   ec);
                    ptr+=bytes_transferred;
                    size-=bytes_transferred;
                    if (ec)
                        return traits_type::eof();
                }
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
                
                // If the new character is eof then our work here is done.
                if (traits_type::eq_int_type(c, traits_type::eof()))
                    return traits_type::not_eof(c);
                
                // Add the new character to the output buffer.
                *pptr() = traits_type::to_char_type(c);
                pbump(1);
                return c;
            }
        }
        
        int sync()
        {
            return overflow(traits_type::eof());
        }
        
        std::streambuf* setbuf(char_type* s, std::streamsize n)
        {
            if (pptr() == pbase() && s == 0 && n == 0)
            {
                unbuffered_ = true;
                setp(0, 0);
                return this;
            }
            
            return 0;
        }
        
    private:
        void init_buffers()
        {
            setg(&get_buffer_[0],
                 &get_buffer_[0] + putback_max,
                 &get_buffer_[0] + putback_max);
            if (unbuffered_)
                setp(0, 0);
            else
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
        }
        
        enum { putback_max = 8 };
        enum { buffer_size = 10 };
        
        StreamDescriptor sd_;
        asio::detail::array<char, buffer_size> get_buffer_;
        asio::detail::array<char, buffer_size> put_buffer_;
        int connect_timeout_=0;
        int read_timeout_=0;
        int write_timeout_=0;
        bool unbuffered_=false;
    };
    
    template<typename StreamDescriptor>
    using streambuf=basic_streambuf<StreamDescriptor>;
}}  // End of namespace fibio::stream

#endif
