//
//  basic_socket_acceptor.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_basic_socket_acceptor_hpp
#define fibio_io_basic_socket_acceptor_hpp

#include <boost/asio/basic_socket_acceptor.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<typename Protocol, typename SocketAcceptorService>
    struct fiberized<boost::asio::basic_socket_acceptor<Protocol, SocketAcceptorService>> : public boost::asio::basic_socket_acceptor<Protocol, SocketAcceptorService>
    {
        typedef boost::asio::basic_socket_acceptor<Protocol, SocketAcceptorService> base_type;

        fiberized() : base_type(fibers::this_fiber::detail::get_io_service()) {}
        fiberized(fiberized &&other)=default;
        
        fiberized(const typename base_type::protocol_type & protocol)
        : base_type(fibers::this_fiber::detail::get_io_service(), protocol)
        {}
        
        fiberized(const typename base_type::endpoint_type & endpoint,
                  bool reuse_addr = true)
        : base_type(fibers::this_fiber::detail::get_io_service(), endpoint, reuse_addr)
        {}
        
        fiberized(const typename base_type::protocol_type & protocol,
                  const typename base_type::native_handle_type & native_socket)
        : base_type(fibers::this_fiber::detail::get_io_service(), protocol, native_socket)
        {}

        void accept(fiberized<boost::asio::basic_stream_socket<Protocol>> & peer)
        {
            boost::system::error_code ec;
            typename base_type::endpoint_type peer_endpoint;
            do_accept(peer, peer_endpoint, ec, true);
        }
        
        boost::system::error_code accept(fiberized<boost::asio::basic_stream_socket<Protocol>> & peer,
                               boost::system::error_code & ec)
        {
            typename base_type::endpoint_type peer_endpoint;
            do_accept(peer, peer_endpoint, ec, false);
            return ec;
        }
        
        void accept(fiberized<boost::asio::basic_stream_socket<Protocol>> & peer,
                    typename base_type::endpoint_type & peer_endpoint)
        {
            boost::system::error_code ec;
            do_accept(peer, peer_endpoint, ec, true);
        }
        
        boost::system::error_code accept(fiberized<boost::asio::basic_stream_socket<Protocol>> & peer,
                               typename base_type::endpoint_type & peer_endpoint,
                               boost::system::error_code & ec)
        {
            do_accept(peer, peer_endpoint, ec, false);
            return ec;
        }
        
        boost::system::error_code do_accept(fiberized<boost::asio::basic_stream_socket<Protocol>> & peer,
                                  typename base_type::endpoint_type & peer_endpoint,
                                  boost::system::error_code & ec,
                                  bool throw_error)
        {
            detail::fiber_async_handler async_handler;
            
            async_handler.start_timer_with_cancelation(accept_timeout_, [this, &peer, &peer_endpoint](){ base_type::cancel(); });
            base_type::async_accept(peer, peer_endpoint, async_handler.get_async_op_handler());
            async_handler.pause_current_fiber();
            
            async_handler.throw_or_return(throw_error, ec);
            return ec;
        }
        
        template<typename Rep, typename Period>
        void set_accept_timeout(const std::chrono::duration<Rep, Period>& timeout_duration)
        { accept_timeout_=std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count(); }
        
        uint64_t accept_timeout_=0;
    };
}}  // End of namespace fibio::io

#endif
