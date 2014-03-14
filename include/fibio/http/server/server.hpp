//
//  server.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014年 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_server_server_hpp
#define fiberized_io_http_server_server_hpp

#include <string>
#include <functional>
#include <system_error>
#include <fibio/stream/iostream.hpp>
#include <fibio/http/server/request.hpp>
#include <fibio/http/server/response.hpp>

namespace fibio { namespace http { namespace server {
    struct server {
        typedef fibio::http::server::request request;
        typedef fibio::http::server::response response;

        struct connection {
            typedef fibio::http::server::request request;
            typedef fibio::http::server::response response;
            
            connection()=default;
            connection(const connection &);
            
            bool recv(request &req, uint64_t timeout=0);
            bool send(response &resp, uint64_t timeout=0);
            
            void close();
            
            template<typename Rep, typename Period>
            bool recv(request &req, const std::chrono::duration<Rep, Period>& timeout_duration)
            { return recv(req, std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count()); }
            
            template<typename Rep, typename Period>
            bool send(response &resp, const std::chrono::duration<Rep, Period>& timeout_duration)
            { return send(resp, std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count()); }
            
            std::string host_;
            stream::tcp_stream stream_;
        };
        
        server(const io::tcp::endpoint &ep, const std::string &host);
        std::error_code accept(connection &sc, uint64_t timeout=0);
        void close();
        
        template<typename Rep, typename Period>
        std::error_code accept(connection &sc, const std::chrono::duration<Rep, Period>& timeout_duration)
        { return accept(sc, std::chrono::duration_cast<std::chrono::microseconds>(timeout_duration).count()); }

        std::string host_;
        io::tcp::acceptor acceptor_;
    };
}}} // End of namespace fibio::http::server

namespace fibio { namespace http {
    typedef server::server http_server;
}}  // End of namespace fibio::http

#endif
