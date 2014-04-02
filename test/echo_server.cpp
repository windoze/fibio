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

typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> watchdog_timer_t;

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

void servant_watchdog(watchdog_timer_t &timer, tcp_stream &s) {
    boost::system::error_code ignore_ec;
    while (s.is_open()) {
        timer.async_wait(asio::yield[ignore_ec]);
        // close the stream if timeout
        if (timer.expires_from_now() <= std::chrono::seconds(0)) {
            s.close();
        }
    }
}

void echo_servant(tcp_stream s) {
    struct conn_guard {
        conn_guard() {connections+=1;}
        ~conn_guard() {connections-=1;}
    } guard;

    watchdog_timer_t timer(asio::get_io_service());
    timer.expires_from_now(std::chrono::seconds(3));
    fiber watchdog(servant_watchdog, std::ref(timer), std::ref(s) );
    hello(s);
    std::string line;
    while (std::getline(s, line)) {
        s << line << std::endl;
        // Reset the watchdog
        timer.expires_from_now(std::chrono::seconds(3));
    }
    // Join the watchdog fiber, make sure it will not use released stream and timer
    timer.expires_from_now(std::chrono::seconds(0));
    timer.cancel();
    watchdog.join();
}

void main_watchdog(tcp_stream_acceptor &acc) {
    while (!should_exit) {
        this_fiber::sleep_for(std::chrono::seconds(1));
    }
    acc.close();
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
    tcp_stream_acceptor acc(address, listen_port);
    fiber watchdog(main_watchdog, std::ref(acc));
    while(true) {
        boost::system::error_code ec;
        tcp_stream s=acc(ec);
        if(ec) break;
        fiber(echo_servant, std::move(s)).detach();
    }
    watchdog.join();
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
