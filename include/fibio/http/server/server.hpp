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
    constexpr unsigned DEFAULT_MAX_KEEP_ALIVE=100;
    constexpr timeout_type DEFAULT_TIMEOUT=std::chrono::seconds(60);
    constexpr timeout_type NO_TIMEOUT=std::chrono::seconds(0);
    
    struct server {
        typedef fibio::http::server_request request;
        typedef fibio::http::server_response response;
        typedef std::function<bool(request &req,
                                   response &resp)> request_handler_type;
        
        struct settings {
            settings(request_handler_type h=[](request &, response &)->bool{ return false; },
                     unsigned short p=80,
                     const std::string &a="0.0.0.0",
                     timeout_type r=DEFAULT_TIMEOUT,
                     timeout_type w=DEFAULT_TIMEOUT,
                     unsigned m=DEFAULT_MAX_KEEP_ALIVE)
            : address(a)
            , port(p)
            , default_request_handler(h)
            , read_timeout(r)
            , write_timeout(w)
            , max_keep_alive(m)
            , ctx(0)
            {
                // read and write timeout must be set or unset at same time
                assert(!((r==NO_TIMEOUT) ^ (w==NO_TIMEOUT)));
            }
                     
            settings(ssl::context &context,
                     request_handler_type h=[](request &, response &)->bool{ return false; },
                     unsigned short p=443,
                     const std::string &a="0.0.0.0",
                     timeout_type r=DEFAULT_TIMEOUT,
                     timeout_type w=DEFAULT_TIMEOUT,
                     unsigned m=DEFAULT_MAX_KEEP_ALIVE)
            : address(a)
            , port(p)
            , default_request_handler(h)
            , read_timeout(r)
            , write_timeout(w)
            , max_keep_alive(m)
            , ctx(&context)
            {
                // read and write timeout must be set or unset at same time
                assert(!((r==NO_TIMEOUT) ^ (w==NO_TIMEOUT)));
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
        boost::system::error_code operator()() {
            try {
                start();
                join();
            } catch(boost::system::system_error &e) {
                return e.code();
            }
            return boost::system::error_code();
        }
        struct impl;
    private:
        impl *engine_;
        std::unique_ptr<fiber> servant_;
        bool ssl_=false;
    };
}}  // End of namespace fibio::http

#endif
