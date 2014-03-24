//
//  test_tcp_stream.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <boost/random.hpp>
#include <fibio/fiber.hpp>
#include <fibio/stream/iostream.hpp>
#include <boost/lexical_cast.hpp>

using namespace fibio;

void child() {
    this_fiber::sleep_for(std::chrono::seconds(1));
    stream::tcp_stream str;
    stream::tcp_stream str1;
    str1.connect("127.0.0.1", "23456");
    std::swap(str, str1);
    str << "hello" << std::endl;
    for(int i=0; i<100; i++) {
        // Receive a random number from server and send it back
        std::string line;
        std::getline(str, line);
        int n=boost::lexical_cast<int>(line);
        str << n << std::endl;
    }
    str.close();
}

void parent() {
    fiber f(child);
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> rand(1,1000);
    
    io::tcp::acceptor acc=io::listen(io::tcp::endpoint(asio::ip::tcp::v4(), 23456), true);
    std::error_code ec;
    stream::tcp_stream str1(io::accept(acc, 0, ec));
    stream::tcp_stream str;
    str.swap(str1);
    assert(!ec);
    std::string line;
    std::getline(str, line);
    assert(line=="hello");
    for (int i=0; i<100; i++) {
        // Ping client with random number
        int n=rand(rng);
        str << n << std::endl;
        std::getline(str, line);
        int r=boost::lexical_cast<int>(line);
        assert(n==r);
    }
    str.close();
    acc.close();
    
    f.join();
}

int main_fiber(int argc, char *argv[]) {
    fiber_group fibers;
    fibers.create_fiber(parent);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
