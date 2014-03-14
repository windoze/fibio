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
#include <fibio/http/client/client.hpp>
#include <boost/lexical_cast.hpp>

using namespace fibio;
using namespace fibio::http;

void the_client() {
    http_client c;
    http_client::request req;
    req.req_line_.url_="/";
    req.set_host("fiberized.io");
    // Default method
    assert(req.req_line_.method_==method::INVALID);
    req.req_line_.method_=method::GET;
    // Default version
    assert(req.req_line_.version_==http_version::INVALID);
    req.req_line_.version_=http_version::HTTP_1_1;
    assert(req.get_persistent()==true);
    assert(req.get_content_length()==0);
    assert(req.headers_["host"]=="fiberized.io");
    req.req_line_.version_=http_version::HTTP_1_0;
    assert(req.get_persistent()==false);
    req.req_line_.version_=http_version::HTTP_1_1;
    
    c.connect("fiberized.io", 80);
    for(int i=0; i<10; i++) {
        http::client::response resp;
        if(c.send_request(req, resp)) {
            // This server returns a 301 response
            assert(resp.status_.version_==http_version::HTTP_1_1);
            assert(resp.status_.status_==status_code::MOVED_PERMANENTLY);
            //std::cout << resp.status_ << std::endl;
            //std::cout << resp.headers_ << std::endl;

            size_t cl=resp.get_content_length();
            std::string s;
            std::stringstream ss;
            ss << resp.body_stream().rdbuf();
            s=ss.str();
            assert(s.size()==cl);
            // Make sure we triggered EOF condition
            resp.body_stream().peek();
            assert(resp.body_stream().eof());
            //std::cout << s << std::endl;
        }
    }
}

int main_fiber(int argc, char *argv[]) {
    std::vector<fiber> fibers;
    for (int i=0; i<10; i++) {
        fibers.push_back(fiber(the_client));
    }
    for (fiber &f : fibers) {
        f.join();
    }
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
