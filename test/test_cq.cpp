//
//  test_cq.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <fibio/fiber.hpp>
#include <fibio/fiberize.hpp>
#include <fibio/concurrent/concurrent_queue.hpp>

using namespace fibio;
concurrent::concurrent_queue<int> cq;

constexpr int children=1000;
constexpr size_t max_num=100;
constexpr long sum=max_num*(max_num+1)/2*children;

barrier bar(children);

void child() {
    for (int i=1; i<=max_num; i++) {
        cq.push(i);
    }
    // Queue is closed only if all child fibers are finished
    if(bar.wait()) cq.close();
}

void parent() {
    for (int n=0; n<children; n++) {
        fiber(child).detach();
    }
    int s=0;
    for (int popped : cq) {
        s+=popped;
    }
    assert(s==sum);
}

int fibio::main(int argc, char *argv[]) {
    this_fiber::get_scheduler().add_worker_thread(3);
    
    fiber_group fibers;
    fibers.create_fiber(parent);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
