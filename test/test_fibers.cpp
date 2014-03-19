//
//  test_fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <fibio/fiber.hpp>

using namespace fibio;

struct data {
    int n=0;
};

void f1(data d) {
    d.n=100;
}

void f2(data &d) {
    d.n=200;
}

int main_fiber(int argc, char *argv[]) {
    std::vector<fiber> fibers;
    
    data d1;
    data d2;
    
    fibers.push_back(fiber(f1, d1));
    fibers.push_back(fiber(f2, std::ref(d2)));
    for (fiber &f : fibers) {
        f.join();
    }
    assert(d1.n==0);
    assert(d2.n=200);
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(1, main_fiber, argc, argv);
}
