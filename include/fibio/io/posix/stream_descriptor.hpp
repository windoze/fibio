//
//  stream_descriptor.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_posix_stream_descriptor_hpp
#define fibio_io_posix_stream_descriptor_hpp

#include <asio/posix/stream_descriptor.hpp>
#include <fibio/io/posix/basic_stream_descriptor.hpp>

namespace fibio {
    typedef io::fiberized<asio::posix::stream_descriptor> posix_stream_descriptor;
}   // End of namespace fibio

#endif
