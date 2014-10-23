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
#include <fibio/fiberize.hpp>

using namespace fibio;
mutex m;
recursive_mutex m1;

void f(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<100; i++) {
        unique_lock<mutex> lock(m);
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
        //printf("f(%d)\n", n);
    }
    //printf("f(%d) exiting\n", n);
}

void f1(int n) {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> three(1,3);
    for (int i=0; i<10; i++) {
        unique_lock<recursive_mutex> lock(m1);
        {
            unique_lock<recursive_mutex> lock(m1);
            this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
            //printf("f1(%d)\n", n);
        }
        this_fiber::sleep_for(std::chrono::milliseconds(three(rng)));
    }
}

timed_mutex tm;

void child() {
    this_fiber::yield();
    bool ret=tm.try_lock_for(std::chrono::milliseconds(10));
    ret=tm.try_lock_for(std::chrono::seconds(2));
    assert(ret);
    tm.unlock();
    //printf("child()\n");
}

void parent() {
    fiber f(child);
    tm.lock();
    this_fiber::sleep_for(std::chrono::seconds(1));
    tm.unlock();
    //printf("parent():1\n");
    f.join();
    //printf("parent():2\n");
}

int fibio::main(int argc, char *argv[]) {
    scheduler::get_instance().add_worker_thread(3);

    fiber_group fibers;
    fibers.create_fiber(parent);
    for (int i=0; i<10; i++) {
        fibers.create_fiber(f, i);
    }
    scheduler::get_instance().add_worker_thread(3);
    for (int i=0; i<10; i++) {
        fibers.create_fiber(f1, i);
    }
    fibers.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
