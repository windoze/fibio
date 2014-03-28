//
//  echo_server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <chrono>
#include <atomic>
#include <fibio/fiber.hpp>
#include <fibio/stream/iostream.hpp>

using namespace fibio;
std::string address("0::0");
std::atomic<unsigned short> listen_port;
std::atomic<bool> should_exit(false);
std::atomic<size_t> connections(0);

void hello(std::ostream &s) {
    s << "Fiberized.IO echo_server listening at " << address << ", port " << listen_port << std::endl;
}

void help_message() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "quit:\tquit the application" << std::endl;
    std::cout << "info:\tshow the number of active connections" << std::endl;
    std::cout << "help:\tshow this message" << std::endl;
}

void console() {
    hello(std::cout);
    help_message();
    std::cin.tie(&std::cout);
    while(std::cin) {
        std::string line;
        std::cout << "> ";
        std::getline(std::cin, line);
        if(line.empty()) continue;
        if(line=="quit") {
            break;
        } else if (line=="info") {
            std::cout << "Active connections: " << connections << std::endl;
        } else if (line=="help") {
            help_message();
        } else {
            std::cout << "Invalid command" << std::endl;
            help_message();
        }
    }
    should_exit=true;
    std::cout << "Exiting..." << std::endl;
}

int main_fiber(int argc, char *argv[]) {
    if (argc<2) {
        std::cerr << "Usage:" << "\t" << argv[0] << " [address] port" << std::endl;
        return 1;
    }
    if (argc==2) {
        listen_port=atoi(argv[1]);
    } else if (argc==3) {
        address=argv[1];
        listen_port=atoi(argv[2]);
    }
    fiber(console).detach();
    auto acc=tcp_stream_acceptor(address, listen_port, std::chrono::seconds(1));
    while(!should_exit) {
        boost::system::error_code ec;
        tcp_stream s=acc(ec);
        if(!ec) {
            fiber([](tcp_stream s){
                struct conn_guard {
                    conn_guard() {connections+=1;}
                    ~conn_guard() {connections-=1;}
                } guard;
                s.set_read_timeout(std::chrono::seconds(3));
                s.set_write_timeout(std::chrono::seconds(3));
                hello(s);
                s << s.rdbuf();
            }, std::move(s)).detach();
        } else if (ec!=make_error_code(boost::system::errc::timed_out)) {
            std::cerr << ec.message() << std::endl;
            return ec.value();
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
