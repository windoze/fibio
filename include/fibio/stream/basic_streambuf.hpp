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
#include <vector>
#include <fibio/fibers/fiber.hpp>

namespace fibio { namespace stream {
    template<typename StreamDescriptor>
    class basic_fibio_streambuf : public std::streambuf {
    public:
        basic_fibio_streambuf()
        : sd_(fibio::fibers::this_fiber::detail::get_io_service())
        , get_buffer_(buffer_size)
        , put_buffer_(buffer_size)
        { init_buffers(); }
        
        basic_fibio_streambuf(basic_fibio_streambuf &&other)
        : sd_(std::move(other.sd_))
        , get_buffer_(std::move(other.get_buffer_))
        , put_buffer_(std::move(other.put_buffer_))
        , connect_timeout_(other.connect_timeout_)
        , read_timeout_(other.read_timeout_)
        , write_timeout_(other.write_timeout_)
        , unbuffered_(other.unbuffered_)
        {}
        
        basic_fibio_streambuf(StreamDescriptor &&sd)
        : sd_(std::move(sd))
        , get_buffer_(buffer_size)
        , put_buffer_(buffer_size)
        { init_buffers(); }
        
        basic_fibio_streambuf(asio::io_service &iosvc)
        : sd_(iosvc)
        , get_buffer_(buffer_size)
        , put_buffer_(buffer_size)
        { init_buffers(); }
        
        /// Destructor flushes buffered data.
        ~basic_fibio_streambuf() {
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
        
        void swap(basic_fibio_streambuf &other) {
            StreamDescriptor temp(sd_.get_io_service());
            temp=std::move(other.sd_);
            other.sd_=std::move(sd_);
            sd_=std::move(temp);
            //std::swap(sd_, other.sd_);
            std::swap(get_buffer_, other.get_buffer_);
            std::swap(put_buffer_, other.put_buffer_);
            std::swap(connect_timeout_, other.connect_timeout_);
            std::swap(read_timeout_, other.read_timeout_);
            std::swap(write_timeout_, other.write_timeout_);
            std::swap(unbuffered_, other.unbuffered_);
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
        pos_type seekoff(off_type off,
                         std::ios_base::seekdir dir,
                         std::ios_base::openmode which) override
        {
            // Only seeking in input buffer from current pos is allowed
            if (which!=std::ios_base::in || dir!=std::ios_base::cur) return pos_type(off_type(-1));
            char_type* newg=gptr()+off;
            // Cannot seek back into put back area
            if (newg <eback()+putback_max) return false;
            // Cannot seek beyond end of get area
            if (newg >=egptr()) return false;
            setg(eback(), newg, egptr());
            return pos_type(off_type(0));
        }
        
        virtual std::streamsize showmanyc() override {
            if ( gptr() == egptr() ) {
                underflow();
            }
            return egptr()-gptr();
        }
        
        int_type underflow() override {
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
        
        int_type overflow(int_type c) override {
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
        
        int sync() override
        {
            return overflow(traits_type::eof());
        }
        
        std::streambuf* setbuf(char_type* s, std::streamsize n) override
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
        void init_buffers() {
            setg(&get_buffer_[0],
                 &get_buffer_[0] + putback_max,
                 &get_buffer_[0] + putback_max);
            if (unbuffered_)
                setp(0, 0);
            else
                setp(&put_buffer_[0], &put_buffer_[0] + put_buffer_.size());
        }
        
        enum { putback_max = 8 };
        // A practical MTU size
        enum { buffer_size = 1500+putback_max };
        
        StreamDescriptor sd_;
        std::vector<char> get_buffer_;
        std::vector<char> put_buffer_;
        int connect_timeout_=0;
        int read_timeout_=0;
        int write_timeout_=0;
        bool unbuffered_=false;
    };
}}  // End of namespace fibio::stream

namespace std {
    template<typename StreamDescriptor>
    void swap(fibio::stream::basic_fibio_streambuf<StreamDescriptor> &lhs, fibio::stream::basic_fibio_streambuf<StreamDescriptor> &rhs) {
        lhs.swap(rhs);
    }
}

#endif
