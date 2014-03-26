//
//  basic_raw_socket.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_basic_raw_socket_hpp
#define fibio_io_basic_raw_socket_hpp

#include <asio/basic_raw_socket.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<typename Protocol, typename StreamSocketService>
    struct fiberized<asio::basic_raw_socket<Protocol, StreamSocketService>> : public asio::basic_raw_socket<Protocol, StreamSocketService>
    {
        typedef asio::basic_raw_socket<Protocol, StreamSocketService> base_type;

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
        
        FIBIO_IMPLEMENT_EXTENDED_CONNECT;
        FIBIO_IMPLEMENT_FIBERIZED_CONNECT;
        FIBIO_IMPLEMENT_FIBERIZED_RECEIVE;
        FIBIO_IMPLEMENT_FIBERIZED_SEND;
        FIBIO_IMPLEMENT_FIBERIZED_RECEIVE_FROM;
        FIBIO_IMPLEMENT_FIBERIZED_SEND_TO;
    };
}}  // End of namespace fibio::io

#endif
