//
//  server.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_server_hpp
#define fibio_http_server_server_hpp

#include <memory>
#include <string>
#include <functional>
#include <system_error>
#include <fibio/stream/iostream.hpp>
#include <fibio/stream/ssl.hpp>
#include <fibio/http/server/request.hpp>
#include <fibio/http/server/response.hpp>

namespace fibio { namespace http {
    constexpr unsigned DEFAULT_KEEP_ALIVE_REQ_PER_CONNECTION=100;
    
    struct server {
        typedef fibio::http::server_request request;
        typedef fibio::http::server_response response;
        typedef std::istream connection;
        typedef std::function<bool(request &req,
                                   response &resp,
                                   connection &conn)> request_handler_type;
        
        struct settings {
            settings(request_handler_type h=[](request &, response &, connection &)->bool{ return false; },
                     const std::string &a="0.0.0.0",
                     unsigned short p=80,
                     timeout_type r=std::chrono::seconds(0),
                     timeout_type w=std::chrono::seconds(0),
                     unsigned m=DEFAULT_KEEP_ALIVE_REQ_PER_CONNECTION)
            : address(a)
            , port(p)
            , default_request_handler(h)
            , read_timeout(r)
            , write_timeout(w)
            , max_keep_alive(m)
            , ctx(0)
            {
                // read and write timeout must be set or unset at same time
                assert(!((r==std::chrono::seconds(0)) ^ (w==std::chrono::seconds(0))));
            }
                     
            settings(ssl::context &context,
                     request_handler_type h=[](request &, response &, connection &)->bool{ return false; },
                     const std::string &a="0.0.0.0",
                     unsigned short p=443,
                     timeout_type r=std::chrono::seconds(0),
                     timeout_type w=std::chrono::seconds(0),
                     unsigned m=DEFAULT_KEEP_ALIVE_REQ_PER_CONNECTION)
            : address(a)
            , port(p)
            , default_request_handler(h)
            , read_timeout(r)
            , write_timeout(w)
            , max_keep_alive(m)
            , ctx(&context)
            {
                // read and write timeout must be set or unset at same time
                assert(!((r==std::chrono::seconds(0)) ^ (w==std::chrono::seconds(0))));
            }

            std::string address;
            unsigned short port;
            request_handler_type default_request_handler;
            timeout_type read_timeout;
            timeout_type write_timeout;
            unsigned max_keep_alive;
            ssl::context *ctx;
        };

        server(settings s);
        ~server();
        void start();
        void stop();
        void join();
        struct impl;
    private:
        impl *engine_;
        std::unique_ptr<fiber> servant_;
        bool ssl_=false;
    };
}}  // End of namespace fibio::http

#endif
