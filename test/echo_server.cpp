//
//  echo_server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/fiber.hpp>
#include <fibio/stream/iostream.hpp>

using namespace fibio;

void servant(stream::tcp_stream s) {
    while(!s.eof()) {
        std::string line;
        std::getline(s, line);
        s << line << std::endl;
    }
}

int main_fiber(int argc, char *argv[]) {
    try {
        io::tcp::acceptor acc=io::listen(io::tcp::endpoint(io::tcp::protocol::v4(), 12345), true);
        while(1) {
            stream::tcp_stream stream(io::accept(acc));
            fiber(servant, std::move(stream)).detach();
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
