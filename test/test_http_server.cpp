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
#include <sstream>
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
    req.url="/";
    // Default method
    assert(req.method==http_method::INVALID);
    req.method=http_method::GET;
    // Default version
    assert(req.version==http_version::INVALID);
    req.version=http_version::HTTP_1_1;
    assert(req.get_content_length()==0);
    
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
            assert(resp.status_code==http_status_code::OK);
            assert(resp.status_message=="OK");
            assert(resp.version==http_version::HTTP_1_1);
            
            size_t cl=resp.content_length;
            std::string s;
            std::stringstream ss;
            ss << resp.body_stream().rdbuf();
            s=ss.str();
            resp.body_stream().peek();
            assert(s.size()==cl);
            //assert(cl==req_body.size()*2);
            assert(resp.body_stream().eof());
        } else {
            assert(false);
        }
    }
}

void servant(http_server::connection sc) {
    http_server::request req;
    int count=0;
    while(sc.recv(req)) {
        //req.write(std::cout);
        //std::cout.flush();

        std::string s;
        if (req.content_length>0 && (req.content_length!=ULONG_MAX)) {
            std::stringstream ss;
            ss << req.body_stream().rdbuf();
            s=ss.str();
        }

        http_server::response resp;
        resp.status_code=http_status_code::OK;
        resp.version=req.version;
        resp.keep_alive=req.keep_alive;
        if(count>=100) resp.keep_alive=false;
        resp.set_content_type("text/html");
        resp.body_stream() << "<HTML><HEAD><TITLE>Test</TITLE></HEAD><BODY><TABLE>" << std::endl;
        for(auto &p: req.headers) {
            resp.body_stream() << "<TR><TD>" << p.first << "</TD><TD>" << p.second << "</TD></TR>" <<std::endl;
        }
        resp.body_stream() << "</TABLE></BODY></HTML>" << std::endl;
        sc.send(resp);
        sc.stream_.flush();
        count++;
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
