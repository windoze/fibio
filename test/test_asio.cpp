//
//  test_asio.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <chrono>
#include <boost/asio/basic_waitable_timer.hpp>
#include <fibio/fiber.hpp>
#include <fibio/fibers/asio/yield.hpp>

using namespace fibio;

typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_t;

void canceler(timer_t &timer) {
    this_fiber::sleep_for(std::chrono::seconds(1));
    timer.cancel();
    this_fiber::sleep_for(std::chrono::seconds(1));
    printf("child exiting...\n");
}

int main_fiber(int argc, char *argv[]) {
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer(asio::get_io_service());
    timer.expires_from_now(std::chrono::seconds(3));
    
    fiber f(canceler, std::ref(timer));
    
    boost::system::error_code ec;
    printf("parent waiting...\n");
    timer.async_wait(asio::yield[ec]);
    assert(ec.value()==boost::system::errc::operation_canceled);
    printf("timer canceled\n");

    f.join();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(1, main_fiber, argc, argv);
}
