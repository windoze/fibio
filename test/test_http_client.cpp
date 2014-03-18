//
//  test_http_client.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <sstream>
#include <fibio/fiber.hpp>
#include <fibio/http/client/client.hpp>

using namespace fibio;
using namespace fibio::http;

void the_client() {
    http_client c;
    http_client::request req;
    req.url.assign("/");
    req.headers.insert(std::make_pair("Host", "tank"));
    // Default method
    assert(req.method==http_method::INVALID);
    req.method=http_method::GET;
    // Default version
    assert(req.version==http_version::INVALID);
    req.version=http_version::HTTP_1_1;
    assert(req.get_content_length()==0);
    //c.set_auto_decompress(true);
    
    req.write(std::cout);
    
    c.connect("tank", 80);
    for(int i=0; i<10; i++) {
        http::client::response resp;
        if(c.send_request(req, resp)) {
            resp.write(std::cout);
            // This server returns a 301 response
            assert(resp.version==http_version::HTTP_1_1);
            //assert(resp.status_code==http_status_code::MOVED_PERMANENTLY);
            assert(resp.status_code==http_status_code::OK);
            //std::cout << resp.status_ << std::endl;
            //std::cout << resp.headers_ << std::endl;

            size_t cl=resp.content_length;
            std::string s;
            std::stringstream ss;
            ss << resp.body_stream().rdbuf();
            s=ss.str();
            std::cout << s;
            std::cout.flush();
            std::cout << s.size() << std::endl;
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
    for (int i=0; i<1; i++) {
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
