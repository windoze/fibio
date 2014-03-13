//
//  test_cv.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <fibio/fiber.hpp>

using namespace fibio;
mutex m;
condition_variable cv;

void child() {
    this_fiber::sleep_for(std::chrono::seconds(1));
    cv.notify_one();
    this_fiber::sleep_for(std::chrono::seconds(1));
    cv.notify_one();
    this_fiber::sleep_for(std::chrono::seconds(1));
    cv.notify_one();
}

void parent() {
    fibio::fiber f(child);
    {
        unique_lock<mutex> lock(m);
        cv.wait(lock);
        assert(lock.owns_lock());
    }
    {
        unique_lock<mutex> lock(m);
        cv_status r=cv.wait_for(lock, std::chrono::microseconds(100));
        assert(r==cv_status::timeout);
        assert(lock.owns_lock());
    }
    {
        unique_lock<mutex> lock(m);
        cv_status r=cv.wait_for(lock, std::chrono::seconds(10));
        assert(r==cv_status::no_timeout);
        assert(lock.owns_lock());
    }
    f.join();
}

int main_fiber(int argc, char *argv[]) {
    std::vector<fiber> fibers;
    fibers.push_back(fiber(parent));
    for (fiber &f : fibers) {
        f.join();
    }
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
