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
    data(int x) : n(x) {}
    data(data &&other) : n(other.n) { other.n=0; }
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
    throw std::runtime_error("exception from child fiber");
}

int main_fiber(int argc, char *argv[]) {
    fiber_group fibers;
    
    data d1(1);
    data d2(2);
    data d3(3);
    data d4(4);
    data d5(5);
    data d6(6);
    
    // Make a copy, d1 won't change
    fibers.create_fiber(f1, d1);
    // Move, d2.n becomes 0
    fibers.create_fiber(f1, std::move(d2));
    // Ref, d3.n will be changed
    fibers.create_fiber(f2, std::ref(d3));
    // Don't compile
    // fibers.push_back(fiber(f2, std::cref(d3)));
    fibers.create_fiber(fiber_entry1(), d4);
    fibers.create_fiber(fiber_entry1(), std::move(d5));
    fibers.create_fiber(fiber_entry2(), std::ref(d6));
    
    // join with rethrow
    fiber fex1(ex);
    try {
        fex1.join(true);
        // joining will throw
        assert(false);
    } catch(std::exception &e) {
        assert(std::string(e.what())=="exception from child fiber");
    }
    
    // join without rethrow
    fiber fex2(ex);
    try {
        // joining will not throw
        fex2.join();
    } catch(...) {
        // No exception should be thrown
        assert(false);
    }
    
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
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    return fiberize(1, main_fiber, argc, argv);
}
