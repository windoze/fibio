//
//  basic_seq_packet_socket.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_basic_seq_packet_socket_hpp
#define fibio_io_basic_seq_packet_socket_hpp

#include <boost/asio/basic_seq_packet_socket.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<typename Protocol, typename StreamSocketService>
    struct fiberized<boost::asio::basic_seq_packet_socket<Protocol, StreamSocketService>> : public boost::asio::basic_seq_packet_socket<Protocol, StreamSocketService>
    {
        typedef boost::asio::basic_seq_packet_socket<Protocol, StreamSocketService> base_type;
        
        fiberized() : base_type(fibers::this_fiber::detail::get_io_service()) {}
        fiberized(fiberized &&other)=default;
        
        fiberized(const typename base_type::protocol_type & protocol)
        : base_type(fibers::this_fiber::detail::get_io_service(), protocol)
        {}
        
        fiberized(const typename base_type::endpoint_type & endpoint)
        : base_type(fibers::this_fiber::detail::get_io_service(), endpoint)
        {}
        
        fiberized(const typename base_type::protocol_type & protocol,
                  const typename base_type::native_handle_type & native_socket)
        : base_type(fibers::this_fiber::detail::get_io_service(), protocol, native_socket)
        {}
        
        FIBIO_IMPLEMENT_FIBERIZED_CONNECT;
        
        // send, only send with flag version is available
        template<typename ConstBufferSequence>
        std::size_t send(const ConstBufferSequence & buffers,
                         boost::asio::socket_base::message_flags flags)
        {
            boost::system::error_code ec;
            return do_send(buffers, flags, ec, true);
        }
        
        template<typename ConstBufferSequence>
        std::size_t send(const ConstBufferSequence & buffers,
                         boost::asio::socket_base::message_flags flags,
                         boost::system::error_code &ec)
        { return do_send(buffers, flags, ec, false); }
        
        template<typename ConstBufferSequence>
        std::size_t do_send(const ConstBufferSequence & buffers,
                            boost::asio::socket_base::message_flags flags,
                            boost::system::error_code &ec,
                            bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &flags](){
                async_handler.start_timer_with_cancelation(send_timeout_, [this](){ base_type::cancel(); });
                base_type::async_send(buffers, flags, async_handler.get_io_handler());
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return async_handler.io_result();
        }
        
        template<typename Rep, typename Period>
        void set_send_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { send_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t send_timeout_=0;
        
        // receive, only receive with flag version is available
        template<typename MutableBufferSequence>
        std::size_t receive(const MutableBufferSequence & buffers,
                            boost::asio::socket_base::message_flags flags)
        {
            boost::system::error_code ec;
            return do_receive(buffers, flags, ec, true);
        }
        
        template<typename MutableBufferSequence>
        std::size_t receive(const MutableBufferSequence & buffers,
                            boost::asio::socket_base::message_flags flags,
                            boost::system::error_code &ec)
        { return do_receive(buffers, flags, ec, false); }
        
        template<typename MutableBufferSequence>
        std::size_t do_receive(const MutableBufferSequence & buffers,
                               boost::asio::socket_base::message_flags flags,
                               boost::system::error_code &ec,
                               bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.run_in_scheduler_context([this, &async_handler, &buffers, &flags](){
                async_handler.start_timer_with_cancelation(receive_timeout_, [this](){ base_type::cancel(); });
                base_type::async_receive(buffers, flags, async_handler.get_io_handler());
            });
            
            async_handler.throw_or_return(throw_error, ec);
            return async_handler.io_result();
        }
        
        template<typename Rep, typename Period>
        void set_receive_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { receive_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t receive_timeout_=0;
        
    };
}}  // End of namespace fibio::io

#endif
