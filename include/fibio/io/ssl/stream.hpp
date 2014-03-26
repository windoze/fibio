//
//  ssl_stream.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_ssl_stream_hpp
#define fibio_ssl_stream_hpp

#include <asio/ssl/stream.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    // To initialize: fiberized<asio::ssl::stream<asio::ip::tcp::socket>>
    template<typename Stream>
    struct fiberized<asio::ssl::stream<Stream>> : public asio::ssl::stream<fiberized<Stream>>
    {
        typedef asio::ssl::stream<fiberized<Stream>> base_type;
        using base_type::base_type;
        
        FIBIO_IMPLEMENT_FIBERIZED_READ_SOME;
        FIBIO_IMPLEMENT_FIBERIZED_WRITE_SOME;
        
        // handshake
        void handshake(typename base_type::handshake_type type) {
            std::error_code ec;
            do_handshake(type, ec, true);
        }
        
        std::error_code handshake(typename base_type::handshake_type type, std::error_code & ec)
        { return do_handshake(type, ec, false); }
        
        std::error_code do_handshake(typename base_type::handshake_type type,
                                     std::error_code & ec,
                                     bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &type](){
                async_handler.start_timer_with_cancelation(handshake_timeout_, [this](){ base_type::cancel(); });
                base_type::async_handshake(type, async_handler.get_async_op_handler());
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename ConstBufferSequence>
        void handshake(typename base_type::handshake_type type,
                       const ConstBufferSequence & buffers)
        {
            std::error_code ec;
            do_handshake(type, buffers, ec, true);
        }
        
        template<typename ConstBufferSequence>
        std::error_code handshake(typename base_type::handshake_type type,
                                  const ConstBufferSequence & buffers,
                                  std::error_code & ec)
        { return do_handshake(type, buffers, ec, false); }
        
        template<typename ConstBufferSequence>
        std::error_code do_handshake(typename base_type::handshake_type type,
                                     const ConstBufferSequence & buffers,
                                     std::error_code & ec,
                                     bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &type, &buffers](){
                async_handler.start_timer_with_cancelation(handshake_timeout_, [this](){ base_type::cancel(); });
                base_type::async_connect(type, buffers, async_handler.get_async_op_handler());
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename Rep, typename Period>
        void set_handshake_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { handshake_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t handshake_timeout_=0;
        
        // shutdown
        void shutdown() {
            std::error_code ec;
            do_shutdown(ec, true);
        }
        
        std::error_code shutdown(std::error_code & ec)
        { return do_shutdown(ec, false); }
        
        std::error_code do_shutdown(std::error_code & ec, bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler](){
                async_handler.start_timer_with_cancelation(shutdown_timeout_, [this](){ base_type::cancel(); });
                base_type::async_shutdown(async_handler.get_async_op_handler());
            });
            
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
