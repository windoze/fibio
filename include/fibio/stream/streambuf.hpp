//
//  streambuf.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_streambuf_hpp
#define fibio_stream_streambuf_hpp

#include <streambuf>
#include <chrono>
#include <vector>
#include <boost/system/error_code.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <fibio/fibers/fiber.hpp>
#include <fibio/fibers/asio/yield.hpp>

namespace boost { namespace asio { namespace ssl {
    template<typename Stream>
    class stream;
}}}

namespace fibio { namespace stream {
    enum duplex_mode {
        full_duplex,
        half_duplex,
    };
    
    template<typename Stream>
    class fiberized_streambuf_base
    : public std::streambuf
    , public Stream
    {
        typedef Stream base_type;
    public:
        typedef Stream stream_type;

        fiberized_streambuf_base()
        : base_type(asio::get_io_service())
        { init_buffers(); }
        
        // For SSL stream, construct with ssl::context
        template<typename Arg>
        fiberized_streambuf_base(Arg &arg)
        : std::streambuf()
        , base_type(asio::get_io_service(), arg)
        { init_buffers(); }

        fiberized_streambuf_base(fiberized_streambuf_base &&other)
        // FIXME: GCC 4.8.1 on Ubuntu, std::streambuf doesn't have move constructor
        // and have copy constructor declared as private, that prevents the generation
        // of default move constructor, and makes only untouched(newly-created)
        // streambuf can be safely moved, as get/put pointers may not be set correctly
        // Seems Clang 3.4 doesn't have the issue
        : /*std::streambuf(std::move(other)),*/
        base_type(std::move(other))
        // we can only move newly create streambuf, anyway
        //, get_buffer_(std::move(other.get_buffer_))
        //, put_buffer_(std::move(other.put_buffer_))
        , unbuffered_(other.unbuffered_)
        , duplex_mode_(other.duplex_mode_)
        { init_buffers(); }
        
        /// Destructor flushes buffered data.
        ~fiberized_streambuf_base() {
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
                boost::system::error_code ec;
                //size_t bytes_transferred=base_type::read_some(boost::asio::buffer(&get_buffer_[0]+ putback_max, buffer_size-putback_max),
                //                                              ec);
                size_t bytes_transferred=base_type::async_read_some(boost::asio::buffer(&get_buffer_[0]+ putback_max,
                                                                                        buffer_size-putback_max),
                                                                    fibers::asio::yield[ec]);
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
            boost::system::error_code ec;
            if (unbuffered_) {
                if (traits_type::eq_int_type(c, traits_type::eof())) {
                    // Nothing to do.
                    return traits_type::not_eof(c);
                } else {
                    char c_=c;
                    //base_type::write_some(boost::asio::buffer(&c_, 1),
                    //                      ec);
                    base_type::async_write_some(boost::asio::buffer(&c_, 1),
                                                fibers::asio::yield[ec]);
                    if (ec)
                        return traits_type::eof();
                    return c;
                }
            } else {
                char *ptr=pbase();
                size_t size=pptr() - pbase();
                while (size > 0)
                {
                    //size_t bytes_transferred=base_type::write_some(boost::asio::buffer(ptr, size),
                    //                                               ec);
                    size_t bytes_transferred=base_type::async_write_some(boost::asio::buffer(ptr, size),
                                                                         fibers::asio::yield[ec]);
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
    
    template<typename Stream>
    class fiberized_streambuf
    : public fiberized_streambuf_base<Stream>
    {
        typedef fiberized_streambuf_base<Stream> base_type;
    public:
        using base_type::base_type;
    };
    
    template<typename Protocol>
    class fiberized_streambuf<boost::asio::basic_stream_socket<Protocol>>
    : public fiberized_streambuf_base<boost::asio::basic_stream_socket<Protocol>>
    {
        typedef fiberized_streambuf_base<boost::asio::basic_stream_socket<Protocol>> base_type;
    public:

        using base_type::base_type;
        
        template<typename Arg>
        boost::system::error_code connect(const Arg &arg) {
            boost::system::error_code ec;
            base_type::async_connect(arg, fibers::asio::yield[ec]);
            return ec;
        }
    };

    template<typename Stream>
    class fiberized_streambuf<boost::asio::ssl::stream<Stream>>
    : public fiberized_streambuf_base<boost::asio::ssl::stream<Stream>>
    {
        typedef fiberized_streambuf_base<boost::asio::ssl::stream<Stream>> base_type;
    public:
        template<typename Context>
        fiberized_streambuf(Context &ctx)
        : base_type(ctx)
        {}
        
        void cancel()
        { base_type::next_layer().cancel(); }
        
        template<typename Arg>
        boost::system::error_code connect(const Arg &arg) {
            boost::system::error_code ec;
            base_type::next_layer().async_connect(arg, fibers::asio::yield[ec]);
            if(ec) return ec;
            base_type::async_handshake(boost::asio::ssl::stream_base::client,
                                       fibers::asio::yield[ec]);
            return ec;
        }
    };
}}  // End of namespace fibio::stream

#endif
