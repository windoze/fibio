//
//  client.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-11.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_client_client_hpp
#define fiberized_io_http_client_client_hpp

#include <string>
#include <functional>
#include <fibio/stream/iostream.hpp>
#include <fibio/http/client/request.hpp>
#include <fibio/http/client/response.hpp>

namespace fibio { namespace http { namespace client {
    struct client {
        typedef fibio::http::client::request request;
        typedef fibio::http::client::response response;
        
        client()=default;
        client(const std::string &server, const std::string &port);
        client(const std::string &server, int port);
        
        std::error_code connect(const std::string &server, const std::string &port);
        std::error_code connect(const std::string &server, int port);
        void disconnect();
        
        void set_auto_decompress(bool c);
        bool get_auto_decompress() const;
        
        bool send_request(request &req, response &resp, uint64_t timeout=0);
        
        template<typename Rep, typename Period>
        bool send_request(request &req, response &resp, const std::chrono::duration<Rep, Period>& timeout_duration)
        { return send_request(req, resp, std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count()); }
        
        std::string server_;
        std::string port_;
        stream::tcp_stream stream_;
        bool auto_decompress_=false;
    };
}}} // End of namespace fibio::http::client

namespace fibio { namespace http {
    typedef client::client http_client;
}}  // End of namespace fibio::http

#endif
