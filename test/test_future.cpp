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
#include <fibio/fiberize.hpp>

using namespace fibio;

int f1(int x) {
    this_fiber::sleep_for(std::chrono::milliseconds(100));
    return x*2;
}

struct f2 {
    f2(int n) : n_(n) {}
    int operator()(int x, int y) const {
        this_fiber::sleep_for(std::chrono::milliseconds(30));
        return x+y+n_;
    }
    int n_;
};

void test_async() {
    assert(async(f2(100), async(f1, 42).get(), async(f1, 24).get()).get()==f2(100)(f1(42), f1(24)));
}

void test_async_executor() {
    async_executor<int> ex(10);
    assert(ex(f2(100), ex(f1, 42).get(), ex(f1, 24).get()).get()==f2(100)(f1(42), f1(24)));
}

void test_async_function() {
    auto af1=make_async(f1);
    auto af2=make_async(f2(100));
    assert(af2(af1(42).get(), af1(24).get()).get()==f2(100)(f1(42), f1(24)));
}

int fibio::main(int argc, char *argv[]) {
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
    
    fiber(test_async).join();
    fiber(test_async_executor).join();
    fiber(test_async_function).join();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
