//
//  echo_server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <chrono>
#include <signal.h>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/signal_set.hpp>
#include <fibio/fiber.hpp>
#include <fibio/stream/iostream.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/asio.hpp>

using namespace fibio;
std::string address("0::0");

// We don't need atomic or any other kind of synchronization as console and
// main_watchdog fibers are always running in the same thread with main_fiber
bool should_exit=false;

// Active connection counter
// Need to be atomic as this counter will be used across multiple threads
std::atomic<size_t> connections(0);

typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> watchdog_timer_t;

void hello(std::ostream &s) {
    s << "Fiberized.IO echo_server listening at " << address << std::endl;
}

void help_message() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "quit:\tquit the application" << std::endl;
    std::cout << "info:\tshow the number of active connections" << std::endl;
    std::cout << "help:\tshow this message" << std::endl;
}

/**
 * console handler, set the exit flag on "quit" command
 */
void console() {
    // Hello message
    hello(std::cout);
    
    // Help message
    help_message();
    
    // Flush `std::cout` before getting input
    std::cin.tie(&std::cout);
    
    // Read a line from `std::cin` and process commands
    while(!should_exit && std::cin) {
        std::string line;
        // Command line prompt
        std::cout << "> ";
        // `std::cout` is automatically flushed before reading as it's tied to std::cout
        std::getline(std::cin, line);
        // Ignore empty line
        if(line.empty()) continue;
        if(line=="quit") {
            // Set the exit flag on `quit` command
            break;
        } else if (line=="info") {
            // Output the number of active connections
            std::cout << "Active connections: " << connections << std::endl;
        } else if (line=="help") {
            // Show help message
            help_message();
        } else {
            std::cout << "Invalid command" << std::endl;
            help_message();
        }
    }
    
    // Set the exit flag
    should_exit=true;
}

/**
 * watchdog for servant, close connection on timeout
 */
void servant_watchdog(watchdog_timer_t &timer, tcp_stream &s) {
    boost::system::error_code ignore_ec;
    while (s.is_open()) {
        timer.async_wait(asio::yield[ignore_ec]);
        // close the stream on timeout
        if (timer.expires_from_now() <= std::chrono::seconds(0)) {
            s.close();
        }
    }
}

/**
 * connection handler fiber
 */
void echo_servant(tcp_stream &s) {
    /**
     * Active connection counter
     */
    struct conn_counter {
        conn_counter() {connections+=1;}
        ~conn_counter() {connections-=1;}
    } counter;

    // Timeout timer
    watchdog_timer_t timer(asio::get_io_service());
    
    // Set connection timeout
    timer.expires_from_now(std::chrono::seconds(3));
    
    // Start watchdog fiber, close connection on timeout
    // ASIO sockets are *not* thread-safe, watchdog must not run in
    // different thread with handler
    fiber watchdog(fiber::attributes(fiber::attributes::stick_with_parent),
                   servant_watchdog,
                   std::ref(timer),
                   std::ref(s) );
    // Hello message
    hello(s);
    
    // Read a line and send it back
    std::string line;
    while (std::getline(s, line)) {
        s << line << std::endl;
        // Reset timeout timer on input
        timer.expires_from_now(std::chrono::seconds(3));
    }
    
    // Ask watchdog to exit
    timer.expires_from_now(std::chrono::seconds(0));
    timer.cancel();
    
    // Make sure watchdog has ended
    watchdog.join();
}

/**
 * Watchdog for main fiber, check the exit flag and signals once every second,
 * close main listener when flag is set or received signal
 */
void signal_watchdog(tcp_listener &l) {
    boost::asio::signal_set ss(asio::get_io_service(),
                               SIGINT,
                               SIGTERM);
#ifdef SIGQUIT
    ss.add(SIGQUIT);
#endif
    
    // Get a future, the future is ready if there is a signal
    future<int> sig=ss.async_wait(asio::use_future);
    
    while(!should_exit
          && sig.wait_for(std::chrono::seconds(1))!=future_status::ready)
    {}
    
    // Set the exit flag
    should_exit=true;
    
    // Stop main listener
    l.stop();
}

/**
 * main fiber, create acceptor to accept incoming connections,
 * starts a servant fiber to handle connection
 */
int fibio::main(int argc, char *argv[]) {
    if (argc!=2) {
        std::cerr << "Usage:" << "\t" << argv[0] << " [address:]port" << std::endl;
        return 1;
    }
    address=argv[1];
    
    // Start more work threads
    this_fiber::get_scheduler().add_worker_thread(3);

    // Start console
    fiber(console).detach();
    
    // Listener
    tcp_listener l(address);

    // Start watchdog
    fiber(signal_watchdog, std::ref(l)).detach();
    
    // Start listener
    int r=l(echo_servant).value();

    std::cout << "Echo server existing..." << std::endl;
    return r;
}
