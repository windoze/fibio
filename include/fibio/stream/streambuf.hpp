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
#include <fibio/io/basic_stream_socket.hpp>
#include <fibio/io/posix/stream_descriptor.hpp>

namespace fibio { namespace stream {
    enum duplex_mode {
        full_duplex,
        half_duplex,
    };

    template<typename... Stream>
    class fiberized_streambuf;

    template<typename... Stream>
    class fiberized_streambuf<io::fiberized<Stream...>>
    : public std::streambuf
    , public io::fiberized<Stream...>
    {
        typedef io::fiberized<Stream...> base_type;
    public:
        typedef io::fiberized<Stream...> stream_type;

        fiberized_streambuf()
        : base_type()
        { init_buffers(); }
        
        fiberized_streambuf(fiberized_streambuf &&other)
        : base_type(std::move(other))
        , get_buffer_(std::move(other.get_buffer_))
        , put_buffer_(std::move(other.put_buffer_))
        , unbuffered_(other.unbuffered_)
        , duplex_mode_(other.duplex_mode_)
        {}
        
        fiberized_streambuf(base_type &&sd)
        : base_type(std::move(sd))
        { init_buffers(); }

        /// Destructor flushes buffered data.
        ~fiberized_streambuf() {
            if (pptr() != pbase())
                overflow(traits_type::eof());
        }
        
        void set_duplex_mode(duplex_mode dm) {
            duplex_mode_=dm;
        }

        duplex_mode get_duplex_mode() const {
            return duplex_mode_;
        }

    protected:
        pos_type seekoff(off_type off,
                         std::ios_base::seekdir dir,
                         std::ios_base::openmode which) override
        {
            // Only seeking in input buffer from current pos is allowed
            if (which!=std::ios_base::in || dir!=std::ios_base::cur) return pos_type(off_type(-1));
            // Do nothing when off=0
            if (off==0) return pos_type(off_type(0));
            char_type* newg=gptr()+off;
            // Cannot seek back into put back area
            if (newg <eback()+putback_max) return pos_type(off_type(-1));
            // Cannot seek beyond end of get area
            if (newg >=egptr()) return pos_type(off_type(-1));
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
            if (duplex_mode_==half_duplex) sync();
            if (gptr() == egptr()) {
                std::error_code ec;
                size_t bytes_transferred=base_type::read_some(asio::buffer(&get_buffer_[0]+ putback_max, buffer_size-putback_max),
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
                    base_type::write_some(asio::buffer(&c_, 1),
                                          ec);
                    if (ec)
                        return traits_type::eof();
                    return c;
                }
            } else {
                char *ptr=pbase();
                size_t size=pptr() - pbase();
                while (size > 0)
                {
                    size_t bytes_transferred=base_type::write_some(asio::buffer(ptr, size),
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
            get_buffer_.resize(buffer_size+putback_max);
            put_buffer_.resize(buffer_size);
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
        enum { buffer_size = 1500 };
        
        std::vector<char> get_buffer_;
        std::vector<char> put_buffer_;
        bool unbuffered_=false;
        duplex_mode duplex_mode_=half_duplex;
    };
}}  // End of namespace fibio::stream

namespace std {
    template<typename... Stream>
    void swap(fibio::stream::fiberized_streambuf<Stream...> &lhs,
              fibio::stream::fiberized_streambuf<Stream...> &rhs) {
        lhs.swap(rhs);
    }
}

#endif
