//
//  datagram_protocol.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_io_local_datagram_protocol_hpp
#define fibio_io_local_datagram_protocol_hpp

#include <asio/local/datagram_protocol.hpp>
#include <fibio/io/basic_datagram_socket.hpp>

namespace fibio {
    typedef io::fiberized<asio::local::datagram_protocol::socket> local_datagram_socket;
}   // End of namespace fibio

#endif
