//
//  echo_server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <fibio/fiber.hpp>
#include <fibio/stream/iostream.hpp>

using namespace fibio;

int main_fiber(int argc, char *argv[]) {
    try {
        auto acc=io::listen(atoi(argv[1]));
        while(1) {
            fiber([](tcp_stream s){
                s << stream::half_duplex << s.rdbuf();
            }, io::accept(acc)).detach();
        }
    } catch (std::system_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
