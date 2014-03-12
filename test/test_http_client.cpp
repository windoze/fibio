//
//  test_mutex.cpp
//  fiberized.io
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <boost/random.hpp>
#include <fibio/fiber.hpp>
#include <strstream>
#include <fibio/http/client/client.hpp>
#include <boost/lexical_cast.hpp>

using namespace fibio;

void http_client() {
    fibio::http::client::client c;
    http::client::request req;
    req.url_="/";
    req.set_host("fiberized.io");
    // Default method
    assert(req.method_==http::common::method::GET);
    // Default version
    assert(req.version_==http::common::http_version::HTTP_1_1);
    assert(req.get_persistent()==true);
    assert(req.get_content_length()==0);
    assert(req.headers_["host"]=="fiberized.io");
    req.version_=http::common::http_version::HTTP_1_0;
    assert(req.get_persistent()==false);
    req.version_=http::common::http_version::HTTP_1_1;
    
    c.connect("fiberized.io", 80);
    for(int i=0; i<10; i++) {
        http::client::response resp;
        if(c.send_request(req, resp)) {
            // This server returns a 301 response
            assert(resp.status_.version_==http::common::http_version::HTTP_1_1);
            assert(resp.status_.status_==http::common::status_code::MOVED_PERMANENTLY);
            //std::cout << resp.status_ << std::endl;
            //std::cout << resp.headers_ << std::endl;

            size_t cl=resp.get_content_length();
            std::string s;
            std::vector<char> buf;
            buf.resize(100);
            while(!resp.body_stream().eof()) {
                resp.body_stream().read(&buf[0], 100);
                size_t sz=resp.body_stream().gcount();
                if (sz==0) {
                    break;
                }
                s.append(buf.begin(),buf.begin()+sz);
            }
            assert(s.size()==cl);
            assert(resp.body_stream().fail());
            assert(resp.body_stream().eof());
            //std::cout << s << std::endl;
        }
    }
}

int main_fiber(int argc, char *argv[]) {
    std::vector<fiber> fibers;
    for (int i=0; i<10; i++) {
        fibers.push_back(fiber(http_client));
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
