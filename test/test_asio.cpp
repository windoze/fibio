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
#include <fibio/asio.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio;

typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> my_timer_t;

void canceler(my_timer_t& timer)
{
    this_fiber::sleep_for(std::chrono::seconds(1));
    timer.cancel();
    this_fiber::sleep_for(std::chrono::seconds(1));
    printf("child exiting...\n");
}

int fibio::main(int argc, char* argv[])
{
    my_timer_t timer(asio::get_io_service());
    boost::system::error_code ec;
    {
        // Async cancelation from another fiber
        timer.expires_from_now(std::chrono::seconds(3));

        fiber f(fiber::attributes(fiber::attributes::stick_with_parent), canceler, std::ref(timer));

        printf("parent waiting...\n");
        timer.async_wait(asio::yield[ec]);
        assert(ec == boost::asio::error::make_error_code(boost::asio::error::operation_aborted));
        printf("timer canceled\n");

        f.join();
    }
    {
        // Use future
        timer.expires_from_now(std::chrono::seconds(3));
        printf("parent waiting again...\n");
        future<void> f = timer.async_wait(asio::use_future);
        future_status status = f.wait_for(std::chrono::seconds(1));
        assert(status == future_status::timeout);
        printf("future timeout\n");
    }
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
