//
//  basic_stream_descriptor.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_posix_basic_stream_descriptor_hpp
#define fibio_io_posix_basic_stream_descriptor_hpp

#include <asio/posix/basic_stream_descriptor.hpp>
#include <fibio/io/detail/wrapper_base.hpp>

namespace fibio { namespace io {
    template<>
    struct fiberized<asio::posix::stream_descriptor> : public asio::posix::stream_descriptor
    {
        typedef asio::posix::stream_descriptor base_type;
        
        fiberized() : base_type(fibers::this_fiber::detail::get_io_service()) {}
        fiberized(fiberized &&other)=default;
        
        fiberized(const typename base_type::native_handle_type & native_descriptor)
        : base_type(fibers::this_fiber::detail::get_io_service(), native_descriptor)
        {}

        FIBIO_IMPLEMENT_FIBERIZED_READ_SOME;
        FIBIO_IMPLEMENT_FIBERIZED_WRITE_SOME;
    };
}}  // End of namespace fibio::io

#endif
