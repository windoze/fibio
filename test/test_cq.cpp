//
//  test_cq.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#include <boost/random.hpp>
#include <fibio/fiber.hpp>
#include <fibio/concurrent/concurrent_queue.hpp>

using namespace fibio;
concurrent::concurrent_queue<int> cq;

void child() {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=1; i<=1000; i++) {
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
        cq.push(i);
    }
    cq.close();
}

void parent() {
    fibio::fiber f(child);
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    int sum=0;
    for (int popped : cq) {
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
        sum+=popped;
    }
    assert(sum==500500);
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
