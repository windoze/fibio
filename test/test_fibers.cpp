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
#include <thread>
#include <fibio/fiber.hpp>

// By defining this, fibio will not replace stream buffers for std streams,
// blocking of std streams will block a thread of scheduler.
// This is needed if you're using multiple schedulers, and more than one of them
// need to access std streams
#define FIBIO_DONT_FIBERIZE_STD_STREAM
#define FIBIO_DONT_USE_DEFAULT_MAIN
#include <fibio/fiberize.hpp>

using namespace fibio;

std::mutex cout_mtx;

/// A test data structure
struct data {
    /// default constructor sets member
    data(int x) : n(x) {}
    /// move constructor clears source
    data(data &&other) : n(other.n) { other.n=0; }
    /// copy constructor
    data(const data &other) : n(other.n) {}
    int n=0;
};

void f1(data d) {
    d.n=100;
}

void f2(data &d) {
    d.n=200;
}

struct fiber_entry1 {
    void operator()(data d) const {
        d.n=300;
    }
};

struct fiber_entry2 {
    void operator()(data &d) const {
        d.n=400;
    }
};

void ex() {
    // Throw something, has not to be std::exception
    throw std::string("exception from child fiber");
}

void test_interrupted1() {
    try {
        this_fiber::sleep_for(std::chrono::seconds(1));
        // fiber should be interrupted
        assert(false);
    } catch(fiber_interrupted) {
        // Interrupted
    }
}

void test_interrupted2() {
    try {
        this_fiber::disable_interruption d1;
        this_fiber::sleep_for(std::chrono::seconds(1));
        // fiber should not be interrupted
        assert(true);
    } catch(fiber_interrupted) {
        assert(false);
    }
}

void test_interrupted3() {
    try {
        this_fiber::disable_interruption d1;
        this_fiber::restore_interruption r1;
        this_fiber::sleep_for(std::chrono::seconds(1));
        // fiber should be interrupted
        assert(false);
    } catch(fiber_interrupted) {
        // Interrupted
        assert(true);
    }
}

void test_interruptor(void (*t)()) {
    fiber f(fiber::attributes(65536), t);
    f.interrupt();
    f.join();
}

int main_fiber(int n) {
    fiber_group fibers;
    
    data d1(1);
    data d2(2);
    data d3(3);
    data d4(4);
    data d5(5);
    data d6(6);
    
    // Plain function as the entry
    // Make a copy, d1 won't change
    fibers.create_fiber(f1, d1);
    // Move, d2.n becomes 0
    fibers.create_fiber(f1, std::move(d2));
    // Ref, d3.n will be changed
    fibers.create_fiber(f2, std::ref(d3));
    // Don't compile
    // fibers.push_back(fiber(f2, std::cref(d3)));
    
    // Functor as the entry
    // Make a copy d4 won't change
    fibers.create_fiber(fiber_entry1(), d4);
    // Move, d5 becomes 0
    fibers.create_fiber(fiber_entry1(), std::move(d5));
    // Ref, d6 will be changed
    fibers.create_fiber(fiber_entry2(), std::ref(d6));
    
    // join with propagation
    fiber fex1(ex);
    try {
        fex1.join(true);
        // joining will throw
        assert(false);
    } catch(std::string &e) {
        assert(e=="exception from child fiber");
    }
    
    // join without propagation, will cause std::terminate() to be called
    /*
    fiber fex2(ex);
    fex2.set_name("ex2");
    try {
        // joining will not throw
        fex2.join();
    } catch(...) {
        // No exception should be thrown
        assert(false);
    }
     */
    
    // Test interruption
    fibers.create_fiber(test_interruptor, test_interrupted1);
    fibers.create_fiber(test_interruptor, test_interrupted2);
    fibers.create_fiber(test_interruptor, test_interrupted3);
    
    fibers.join_all();
    
    // d1.n unchanged
    assert(d1.n==1);
    // d2.n cleared
    assert(d2.n==0);
    // d3.n changed
    assert(d3.n=200);
    // d4.n unchanged
    assert(d4.n==4);
    // d5.n cleared
    assert(d5.n==0);
    // d6.n changed
    assert(d6.n=400);
    // We need a lock as the std::cout is shared across multiple schedulers
    std::lock_guard<std::mutex> lk(cout_mtx);
    std::cout << "main_fiber in scheduler[" << n << "] exiting" << std::endl;
    return 0;
}

int main() {
    // Create 10 schedulers from a fiber belongs to the default scheduler
    std::vector<std::thread> threads;
    for(size_t i=0; i<10; i++) {
        threads.emplace_back([i](){
            fibio::fiberize_with_sched(fibio::scheduler(), main_fiber, i);
            std::lock_guard<std::mutex> lk(cout_mtx);
            std::cout << "scheduler[" << i << "] destroyed" << std::endl;
        });
    }
    for(auto &t : threads) t.join();
    
    std::cout << "main thread exiting" << std::endl;
    return 0;
}
