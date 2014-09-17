//
//  test_fibers.cpp
//  fibio
//
//  Created by Chen Xu on 14-9-17.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <fibio/fiber.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio;

void f(int x) {
    const int count=1000;
    static fiber_specific_ptr<int> p;
    p.reset(new int());
    for(int i=0; i<count; i++) {
        this_fiber::yield();
        *p += x;
    }
    assert(*p==x*count);
}

int fibio::main(int argc, char *argv[]) {
    scheduler::get_instance().add_worker_thread(3);
    fiber_group fibers;
    for(int n=0; n<100; n++) {
        fibers.create_fiber(f, n);
    }
    fibers.join_all();
    
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
