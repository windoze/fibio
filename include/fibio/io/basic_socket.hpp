//
//  socket.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_socket_hpp
#define fibio_socket_hpp

#include <asio/basic_socket.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<typename Protocol, typename StreamSocketService>
    struct fiberized<asio::basic_socket<Protocol, StreamSocketService>> : public asio::basic_socket<Protocol, StreamSocketService>
    {
        typedef asio::basic_socket<Protocol, StreamSocketService> base_type;
        
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
    };
}}  // End of namespace fibio::io

#endif
