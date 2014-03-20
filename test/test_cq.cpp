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
#include <fibio/concurrent/concurrent_queue.hpp>

using namespace fibio;
concurrent::concurrent_queue<int> cq;
constexpr int count=1000;
barrier bar(count);

void child() {
    for (int i=1; i<=1000; i++) {
        cq.push(i);
    }
    // Queue is closed only if all child fibers are finished
    if(bar.wait()) cq.close();
}

void parent() {
    for (int n=0; n<count; n++) {
        fiber(child).detach();
    }
    int sum=0;
    for (int popped : cq) {
        sum+=popped;
    }
    assert(sum==500500 * count);
}

int main_fiber(int argc, char *argv[]) {
    fiber_group fibers;
    fibers.create_fiber(parent);
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fibio::fibers::fiberize(4, main_fiber, argc, argv);
}
