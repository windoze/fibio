//
//  stream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_ssl_stream_hpp
#define fibio_io_ssl_stream_hpp

#include <boost/asio/ssl.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    // To initialize: fiberized<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>
    // which actually constructs a fiberized ssl stream with a embedded fiberized socket
    // fiberized ssl stream makes handshake/shutdown/read_some/write_some work with fibers
    // embedded fiberized socket is required to make streambuf/iostream work
    template<typename Stream>
    struct fiberized<boost::asio::ssl::stream<Stream>> : public boost::asio::ssl::stream<fiberized<Stream>>
    {
        typedef boost::asio::ssl::stream<fiberized<Stream>> base_type;
        
        fiberized(boost::asio::ssl::context &ctx)
        : base_type(fibers::this_fiber::detail::get_io_service(), ctx)
        {}
        
        FIBIO_IMPLEMENT_EXTENDED_CONNECT;
        FIBIO_IMPLEMENT_FIBERIZED_CONNECT;
        FIBIO_IMPLEMENT_FIBERIZED_READ_SOME;
        FIBIO_IMPLEMENT_FIBERIZED_WRITE_SOME;
        
        // handshake
        void handshake(typename base_type::handshake_type type) {
            boost::system::error_code ec;
            do_handshake(type, ec, true);
        }
        
        boost::system::error_code handshake(typename base_type::handshake_type type, boost::system::error_code & ec)
        { return do_handshake(type, ec, false); }
        
        boost::system::error_code do_handshake(typename base_type::handshake_type type,
                                     boost::system::error_code & ec,
                                     bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.start_timer_with_cancelation(handshake_timeout_, [this](){ this->lowest_layer().cancel(); });
            base_type::async_handshake(type, async_handler.get_async_op_handler());
            async_handler.pause_current_fiber();
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename ConstBufferSequence>
        void handshake(typename base_type::handshake_type type,
                       const ConstBufferSequence & buffers)
        {
            boost::system::error_code ec;
            do_handshake(type, buffers, ec, true);
        }
        
        template<typename ConstBufferSequence>
        boost::system::error_code handshake(typename base_type::handshake_type type,
                                  const ConstBufferSequence & buffers,
                                  boost::system::error_code & ec)
        { return do_handshake(type, buffers, ec, false); }
        
        template<typename ConstBufferSequence>
        boost::system::error_code do_handshake(typename base_type::handshake_type type,
                                     const ConstBufferSequence & buffers,
                                     boost::system::error_code & ec,
                                     bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.start_timer_with_cancelation(handshake_timeout_, [this](){ this->lowest_layer().cancel(); });
            base_type::async_handshake(type, buffers, async_handler.get_async_op_handler());
            async_handler.pause_current_fiber();
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename Rep, typename Period>
        void set_handshake_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { handshake_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t handshake_timeout_=0;
        
        // shutdown
        void shutdown() {
            boost::system::error_code ec;
            do_shutdown(ec, true);
        }
        
        boost::system::error_code shutdown(boost::system::error_code & ec)
        { return do_shutdown(ec, false); }
        
        boost::system::error_code do_shutdown(boost::system::error_code & ec, bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.start_timer_with_cancelation(shutdown_timeout_, [this](){ this->lowest_layer().cancel(); });
            base_type::async_shutdown(async_handler.get_async_op_handler());
            async_handler.pause_current_fiber();
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename Rep, typename Period>
        void set_shutdown_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { shutdown_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t shutdown_timeout_=0;
    };
}}  // End of namespace fibio::io

#endif
