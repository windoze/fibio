//
//  test_future.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <fibio/fiber.hpp>
#include <fibio/future.hpp>

using namespace fibio;

int main_fiber(int argc, char *argv[]) {
    // future from a packaged_task
    packaged_task<int()> task([](){ return 7; }); // wrap the function
    future<int> f1 = task.get_future();  // get a future
    fiber(std::move(task)).detach(); // launch on a fiber
    
    // future from an async()
    future<int> f2 = async([](){ return 8; });
    
    // future from a promise
    promise<int> p;
    future<int> f3 = p.get_future();
    fiber([](promise<int> p){
        p.set_value(9);
    }, std::move(p)).detach();
    
    f1.wait();
    f2.wait();
    f3.wait();
    int n1=f1.get();
    int n2=f2.get();
    int n3=f3.get();
    assert(n1==7);
    assert(n2==8);
    assert(n3==9);
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(4, main_fiber, argc, argv);
}
