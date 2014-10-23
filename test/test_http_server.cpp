//
//  test_http_client.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fibio/fiber.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/http/client/client.hpp>
#include <fibio/http/server/server.hpp>
#include <fibio/http/server/routing.hpp>

using namespace fibio;
using namespace fibio::http;
using namespace fibio::http::common;

void the_client() {
    client c;
    if(c.connect("127.0.0.1", 23456)) {
        assert(false);
    }
    
    client::request req;
    client::response resp;
    bool ret;
    
    //std::cout << "GET /" << std::endl;
    ret=c.send_request(make_request(req, "/"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    assert(resp.status_message=="OK");
    assert(resp.version==http_version::HTTP_1_1);
    
    //std::cout << "GET /index.html" << std::endl;
    ret=c.send_request(make_request(req, "/index.html"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /index.htm" << std::endl;
    ret=c.send_request(make_request(req, "/index.htm"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /index.php" << std::endl;
    ret=c.send_request(make_request(req, "/index.php"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "GET /test1/123/test2" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123/test2"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /test1/123" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "POST /test1/123/test2" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123/test2", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::BAD_REQUEST);
    
    //std::cout << "POST /test2/123" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "POST /test2/123/abc/xyz" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123/abc/xyz", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /test2/123" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret=c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::FORBIDDEN);

    //std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret=c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url.html"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
}

void the_url_client() {
    url_client c;
    client::response &resp=c.request("http://127.0.0.1:23456/");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.html");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.htm");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/index.php");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test1/123/test2");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/test1/123");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test1/123/test2", "this is request body");
    assert(resp.status_code==http_status_code::BAD_REQUEST);
    c.request("http://127.0.0.1:23456/test2/123", "this is request body");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/test2/123/abc/xyz", "this is request body");
    assert(resp.status_code==http_status_code::OK);
    c.request("http://127.0.0.1:23456/test2/123");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("http://127.0.0.1:23456/test3/with/a/long/and/stupid/url");
    assert(resp.status_code==http_status_code::FORBIDDEN);
    c.request("http://127.0.0.1:23456/test3/with/a/long/and/stupid/url.html");
    assert(resp.status_code==http_status_code::OK);
}

bool handler(server::request &req, server::response &resp) {
    resp.headers.insert({"Header1", "Value1"});
    // Write all headers back in a table
    resp.set_content_type("text/html");
    resp.body_stream() << "<HTML><HEAD><TITLE>Test</TITLE></HEAD><BODY>"<< std::endl;
    resp.body_stream() << "<H1>Request Info</H1>" << std::endl;
    
    resp.body_stream() << "<TABLE>" << std::endl;
    resp.body_stream() << "<TR><TD>URL</TD><TD>" << req.url << "</TD></TR>" << std::endl;
    resp.body_stream() << "<TR><TD>Schema</TD><TD>" << req.parsed_url.schema << "</TD></TR>" << std::endl;
    resp.body_stream() << "<TR><TD>Port</TD><TD>" << req.parsed_url.port << "</TD></TR>" << std::endl;
    resp.body_stream() << "<TR><TD>Path</TD><TD>" << req.parsed_url.path << "</TD></TR>" << std::endl;
    resp.body_stream() << "<TR><TD>Query</TD><TD>" << req.parsed_url.query << "</TD></TR>" << std::endl;
    resp.body_stream() << "<TR><TD>User Info</TD><TD>" << req.parsed_url.userinfo << "</TD></TR>" << std::endl;
    resp.body_stream() << "</TABLE>" << std::endl;
    
    resp.body_stream() << "<H1>Headers</H1>" << std::endl;
    resp.body_stream() << "<TABLE>" << std::endl;
    for(auto &p: req.headers) {
        resp.body_stream() << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>" <<std::endl;
    }
    resp.body_stream() << "</TABLE>" << std::endl;
    
    resp.body_stream() << "<H1>Parameters</H1>" << std::endl;
    resp.body_stream() << "<TABLE>" << std::endl;
    for(auto &p: req.params) {
        resp.body_stream() << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>" <<std::endl;
    }
    resp.body_stream() << "</TABLE>" << std::endl;
    
    resp.body_stream() << "<H1>Query</H1>" << std::endl;
    resp.body_stream() << "<TABLE>" << std::endl;
    for(auto &p: req.parsed_url.query_params) {
        resp.body_stream() << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>" <<std::endl;
    }
    resp.body_stream() << "</TABLE>" << std::endl;
    
    resp.body_stream() << "</BODY></HTML>" << std::endl;
    return true;
}

void http_server() {
    server svr(server::settings{
        route((path_("/")
               || path_("/index.html")
               || path_("/index.htm")) >> handler,
              GET("/test1/:id/test2") >> handler,
              POST("/test2/*p") >> handler,
              (path_("/test3/*p") && url_(iends_with{".html"})) >> handler,
              path_("/test3/*") >> stock_handler{http_status_code::FORBIDDEN},
              !method_is(http_method::GET) >> stock_handler{http_status_code::BAD_REQUEST}),
        23456,
        "127.0.0.1",
    });
    svr.start();
    {
        // Create some clients, do some requests
        fiber_group fibers;
        size_t n=10;
        for (int i=0; i<n; i++) {
            fibers.create_fiber(the_client);
            fibers.create_fiber(the_url_client);
        }
        fibers.join_all();
    }
    svr.stop();
    svr.join();
}

void the_ssl_client() {
    client c;
    ssl::context ctx(ssl::context::tlsv1_client);
    if(c.connect(ctx, "127.0.0.1", 23457)) {
        assert(false);
    }
    
    client::request req;
    client::response resp;
    bool ret;
    
    //std::cout << "GET /" << std::endl;
    ret=c.send_request(make_request(req, "/"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    assert(resp.status_message=="OK");
    assert(resp.version==http_version::HTTP_1_1);
    
    //std::cout << "GET /index.html" << std::endl;
    ret=c.send_request(make_request(req, "/index.html"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /index.htm" << std::endl;
    ret=c.send_request(make_request(req, "/index.htm"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /index.php" << std::endl;
    ret=c.send_request(make_request(req, "/index.php"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "GET /test1/123/test2" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123/test2"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /test1/123" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "POST /test1/123/test2" << std::endl;
    ret=c.send_request(make_request(req, "/test1/123/test2", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::BAD_REQUEST);
    
    //std::cout << "POST /test2/123" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "POST /test2/123/abc/xyz" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123/abc/xyz", "this is request body"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
    
    //std::cout << "GET /test2/123" << std::endl;
    ret=c.send_request(make_request(req, "/test2/123"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::NOT_FOUND);
    
    //std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret=c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::FORBIDDEN);
    
    //std::cout << "GET /test3/with/a/long/and/stupid/url" << std::endl;
    ret=c.send_request(make_request(req, "/test3/with/a/long/and/stupid/url.html"), resp);
    assert(ret);
    assert(resp.status_code==http_status_code::OK);
}

void the_ssl_url_client() {
    url_client c;
    client::response &resp=c.request("https://127.0.0.1:23457/");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.html");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.htm");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/index.php");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test1/123/test2");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/test1/123");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test1/123/test2", "this is request body");
    assert(resp.status_code==http_status_code::BAD_REQUEST);
    c.request("https://127.0.0.1:23457/test2/123", "this is request body");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/test2/123/abc/xyz", "this is request body");
    assert(resp.status_code==http_status_code::OK);
    c.request("https://127.0.0.1:23457/test2/123");
    assert(resp.status_code==http_status_code::NOT_FOUND);
    c.request("https://127.0.0.1:23457/test3/with/a/long/and/stupid/url");
    assert(resp.status_code==http_status_code::FORBIDDEN);
    c.request("https://127.0.0.1:23457/test3/with/a/long/and/stupid/url.html");
    assert(resp.status_code==http_status_code::OK);
}

void https_server() {
    boost::system::error_code ec;
    ssl::context ctx(ssl::context::tlsv1_server);
    ctx.set_options(ssl::context::default_workarounds
                    | ssl::context::no_sslv2
                    | ssl::context::single_dh_use);
    ctx.set_password_callback([](std::size_t, ssl::context::password_purpose)->std::string{ return "test"; });
    ctx.use_certificate_chain_file("server.pem", ec);
    assert(!ec);
    ctx.use_private_key_file("server.pem", ssl::context::pem, ec);
    assert(!ec);
    ctx.use_tmp_dh_file("dh512.pem", ec);
    assert(!ec);
    server svr(server::settings{ctx,
        route((path_("/")
               || path_("/index.html")
               || path_("/index.htm")) >> handler,
              GET("/test1/:id/test2") >> handler,
              POST("/test2/*p") >> handler,
              (path_("/test3/*p") && url_(iends_with{".html"})) >> handler,
              path_("/test3/*") >> stock_handler{http_status_code::FORBIDDEN},
              !method_is(http_method::GET) >> stock_handler{http_status_code::BAD_REQUEST}),
        23457,
        "127.0.0.1"
    });
    svr.start();
    {
        // Create some clients, do some requests
        fiber_group fibers;
        size_t n=10;
        for (int i=0; i<n; i++) {
            fibers.create_fiber(the_ssl_client);
            fibers.create_fiber(the_ssl_url_client);
        }
        fibers.join_all();
    }
    svr.stop();
    svr.join();
}

int fibio::main(int argc, char *argv[]) {
    scheduler::get_instance().add_worker_thread(3);
    fiber_group fibers;
    fibers.create_fiber(http_server);
    fibers.create_fiber(https_server);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
