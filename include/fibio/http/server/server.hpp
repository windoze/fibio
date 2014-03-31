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
            connection(connection &&other)=default;
            connection(const connection &)=delete;
            
            bool recv(request &req);
            bool send(response &resp);
            
            void close();
            
            std::string host_;
            stream::tcp_stream stream_;
        };
        
        server(const std::string &addr, unsigned short port, const std::string &host);
        server(unsigned short port, const std::string &host);
        boost::system::error_code accept(connection &sc);
        void close();
        
        std::string host_;
        tcp_stream_acceptor acceptor_;
    };
}}} // End of namespace fibio::http::server

namespace fibio { namespace http {
    typedef server::server http_server;
}}  // End of namespace fibio::http

#endif
