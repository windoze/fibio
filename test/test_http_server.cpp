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
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/random.hpp>
#include <fibio/fiber.hpp>
#include <strstream>
#include <boost/lexical_cast.hpp>
#include <fibio/http/client/client.hpp>
#include <fibio/http/server/server.hpp>

using namespace fibio;
using namespace fibio::http;

std::atomic<bool> should_exit;

void the_client() {
    http_client c;
    http_client::request req;
    std::string req_body("hello");
    req.set_url("/");
    req.set_host("localhost:23456");
    // Default method
    assert(req.get_method()==method::INVALID);
    req.set_method(http::common::method::GET);
    // Default version
    assert(req.get_http_version()==http_version::INVALID);
    assert(req.get_content_length()==0);
    assert(req.headers_["host"]=="localhost:23456");
    req.set_http_version(http_version::HTTP_1_0);
    assert(req.get_persistent()==false);
    req.set_http_version(http_version::HTTP_1_1);
    assert(req.get_persistent()==true);
    req.body_stream() << "hello";
    req.body_stream().flush();
    assert(req.get_content_length()==5);
    
    if(c.connect("127.0.0.1", 23456)) {
        assert(false);
    }
    
    for(int i=0; i<100; i++) {
        http_client::response resp;
        if(c.send_request(req, resp)) {
            if (!c.stream_.is_open()) {
                //std::cout << "XXXXXXXX1" << std::endl;
                assert(false);
            }
            if (c.stream_.eof()) {
                //std::cout << "XXXXXXXX2" << std::endl;
                assert(false);
            }
            // This server returns a 200 response
            assert(resp.get_status_code()==status_code::OK);
            assert(resp.get_http_version()==http_version::HTTP_1_1);
            
            size_t cl=resp.get_content_length();
            std::string s;
            std::stringstream ss;
            ss << resp.body_stream().rdbuf();
            s=ss.str();
            resp.body_stream().peek();
            assert(s.size()==cl);
            assert(cl==req_body.size()*2);
            assert(resp.body_stream().eof());
        } else {
            std::cout << "XXXXXXXX3" << std::endl;
        }
    }
}

void servant(http_server::connection sc) {
    http_server::request req;
    while(sc.recv(req)) {
        std::string s;
        std::stringstream ss;
        ss << req.body_stream().rdbuf();
        s=ss.str();

        http_server::response resp;
        resp.set_status_code(status_code::OK);
        resp.set_http_version(req.get_http_version());
        resp.set_persistent(req.get_persistent());
        resp.set_status_msg("OK");
        resp.headers_["Content-Type"]="text/plain";
        resp.body_stream() << s << s;
        sc.send(resp);
    }
}

void the_server() {
    http_server svr(io::tcp::endpoint(asio::ip::tcp::v4(), 23456), "localhost:23456");
    std::error_code ec;
    while (1) {
        http_server::connection sc;
        ec=svr.accept(sc, std::chrono::seconds(1));
        // Why ec has system_category instead of generic_category?
        if (ec.value()==std::make_error_code(std::errc::timed_out).value()) {
            // No one tried to connect in last 1 second
            // Check exit flag
            if (should_exit) {
                return;
            }
            continue;
        }
        if (ec) {
            std::cout << ec << std::endl;
            continue;
        }
        fiber s([sc](){ servant(sc); });
        s.detach();
    }
}

int main_fiber(int argc, char *argv[]) {
    should_exit=false;
    std::vector<fiber> fibers;
    fiber(the_server).detach();
    this_fiber::sleep_for(std::chrono::seconds(1));
    size_t n=10;
    for (int i=0; i<n; i++) {
        fibers.push_back(fiber(the_client));
    }
    for (fiber &f : fibers) {
        f.join();
        n--;
        if(n<=0) should_exit=true;
    }
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
