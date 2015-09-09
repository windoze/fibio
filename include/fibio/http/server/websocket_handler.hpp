//
//  websocket_handler.hpp
//  fibio-http
//
//  Created by Chen Xu on 15/08/28.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_websocket_handler_hpp
#define fibio_http_server_websocket_handler_hpp

#include <fibio/http/common/websocket.hpp>
#include <fibio/http/server/server.hpp>

namespace fibio {
namespace http {
namespace websocket {

struct handler
{
    handler(connection_handler handler);

    handler(const std::string protocol, connection_handler handler);

    bool operator()(server::request& req, server::response& resp);

    std::string protocol_;
    connection_handler handler_;
};

} // End of namespace websocket
} // End of namespace http
} // End of namespace fibio

#endif // !defined(fibio_http_server_websocket_handler_hpp)
