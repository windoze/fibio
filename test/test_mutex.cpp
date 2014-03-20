//
//  test_mutex.cpp
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

using namespace fibio;
mutex m;
recursive_mutex m1;

void f(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<100; i++) {
        unique_lock<mutex> lock(m);
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
    }
}

void f1(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<100; i++) {
        unique_lock<recursive_mutex> lock(m1);
        {
            unique_lock<recursive_mutex> lock(m1);
            this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
        }
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
    }
}

int main_fiber(int argc, char *argv[]) {
    std::vector<fiber> fibers;
    for (int i=0; i<100; i++) {
        fibers.push_back(fiber(f, i));
    }
    for (int i=0; i<100; i++) {
        fibers.push_back(fiber(f1, i));
    }
    for (fiber &f : fibers) {
        f.join();
    }
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
