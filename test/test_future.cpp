//
//  test_future.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <boost/lexical_cast.hpp>
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

void test_future() {
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
}

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

void test_wait_for_any1() {
    future<void> f0=async([](){
        this_fiber::sleep_for(std::chrono::seconds(1));
    });
    future<int> f1=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(100));
        return 100;
    });
    future<double> f2=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(300));
        return 100.5;
    });
    size_t n=wait_for_any(f0, f1, f2);
    // 2nd future should be ready
    assert(n==1);
}

void test_wait_for_any2() {
    std::vector<future<void>> fv;
    for (size_t i=0; i<10; i++) {
        fv.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(100*(i+1)));
        }));
    }
    std::vector<future<void>>::const_iterator i=wait_for_any(fv.cbegin(), fv.cend());
    // 1st future should be ready
    assert(i==fv.begin());
}

void test_wait_for_any3() {
    future<void> f0=async([](){
        this_fiber::sleep_for(std::chrono::seconds(1));
    });
    shared_future<double> f1=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(300));
        return 100.5;
    }).share();
    future<int> f2=make_ready_future(100);
    size_t n=wait_for_any(std::tie(f0, f1, f2));
    // 3rd future should be ready
    assert(n==2);
}

void test_wait_for_all1() {
    auto start=std::chrono::system_clock::now();
    future<void> f0=async([](){
        this_fiber::sleep_for(std::chrono::seconds(1));
    });
    future<int> f1=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(100));
        return 100;
    });
    future<double> f2=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(300));
        return 100.5;
    });
    wait_for_all(f0, f1, f2);
    auto stop=std::chrono::system_clock::now();
    std::chrono::system_clock::duration dur=stop-start;
    assert(dur>=std::chrono::seconds(1));
}

void test_wait_for_all2() {
    constexpr size_t c=10;
    auto start=std::chrono::system_clock::now();
    std::vector<future<void>> fv;
    for (size_t i=0; i<c; i++) {
        fv.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(100*(i+1)));
        }));
    }
    wait_for_all(fv.cbegin(), fv.cend());
    auto stop=std::chrono::system_clock::now();
    std::chrono::system_clock::duration dur=stop-start;
    assert(dur>=std::chrono::milliseconds(100*c));
}

void test_wait_for_all3() {
    auto start=std::chrono::system_clock::now();
    auto f0=async([](){
        this_fiber::sleep_for(std::chrono::seconds(1));
    });
    auto f1=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(100));
        return 100;
    });
    auto f2=async([](){
        this_fiber::sleep_for(std::chrono::milliseconds(300));
        return 100.5;
    });
    wait_for_all(std::tie(f0, f1, f2));
    auto stop=std::chrono::system_clock::now();
    std::chrono::system_clock::duration dur=stop-start;
    assert(dur>=std::chrono::seconds(1));
}

void test_async_wait_for_any1() {
    std::vector<future<void>> fv;
    for (size_t i=0; i<10; i++) {
        fv.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(100*(i+1)));
        }));
    }
    auto f=async_wait_for_any(fv.cbegin(), fv.cend());
    // 1st future should be ready
    assert(f.get()==fv.begin());
}

void test_async_wait_for_any2() {
    std::vector<future<void>> fv1;
    for (int i=0; i<10; i++) {
        fv1.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(100*(i+1)));
        }));
    }
    auto f1=async_wait_for_any(fv1.cbegin(), fv1.cend());

    std::vector<future<int>> fv2;
    for (int i=0; i<10; i++) {
        fv2.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(90*(i+1)));
            return i;
        }));
    }
    auto f2=async_wait_for_any(fv2.cbegin(), fv2.cend());
    wait_for_all(f1, f2);
    // 1st future should be ready
    assert(f1.get()==fv1.begin());
    assert(f2.get()==fv2.begin());
}

void test_async_wait_for_all1() {
    auto start=std::chrono::system_clock::now();
    std::vector<future<void>> fv1;
    int c=10;
    for (int i=0; i<c; i++) {
        fv1.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(100*(i+1)));
        }));
    }
    auto f1=async_wait_for_all(fv1.cbegin(), fv1.cend());
    
    std::vector<future<int>> fv2;
    for (int i=0; i<c; i++) {
        fv2.push_back(async([i](){
            this_fiber::sleep_for(std::chrono::milliseconds(90*(i+1)));
            return i;
        }));
    }
    auto f2=async_wait_for_all(fv2.cbegin(), fv2.cend());
    wait_for_all(f1, f2);
    auto stop=std::chrono::system_clock::now();
    std::chrono::system_clock::duration dur=stop-start;
    assert(dur>=std::chrono::milliseconds(100*c));
}

void test_then1() {
    promise<int> p;
    auto f0=p.get_future();
    auto f1=f0.then([](future<int> &f){ return boost::lexical_cast<std::string>(f.get()); });
    p.set_value(100);
    assert(f1.get()==std::string("100"));
}

void test_then2() {
    auto f=async([](){ return 100; })
        .then([](future<int> &f){ return boost::lexical_cast<std::string>(f.get()); })
        .then([](future<std::string> &f){ return boost::lexical_cast<int>(f.get()); })
    ;
    assert(f.get()==100);
}

void test_then3() {
    promise<int> p;
    auto f0=p.get_future().share();
    auto f1=f0.then([](shared_future<int> &f){ return boost::lexical_cast<std::string>(f.get()); });
    p.set_value(100);
    assert(f1.get()==std::string("100"));
}

void test_then4() {
    auto f=async([](){ return 100; }).share()
        .then([](shared_future<int> &f){ return boost::lexical_cast<std::string>(f.get()); }).share()
        .then([](shared_future<std::string> &f){ return boost::lexical_cast<int>(f.get()); }).share()
    ;
    assert(f.get()==100);
}

int thr_func(int x) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return x*10;
}

void thr_func1() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void dot() {
    for (int i=0; i<30; i++) {
        std::cout << i << '.' << std::endl;
        this_fiber::sleep_for(std::chrono::milliseconds(100));
    }
}

void test_foreign_thread_pool() {
    fiber f(dot);
    foreign_thread_pool pool;
    struct thr_op {
        int operator()(int x) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return x*10;
        }
    };
    assert(pool(thr_func, 42)==420);
    std::cout << "check" << std::endl;
    assert(pool(thr_op(), 42)==420);
    std::cout << "check" << std::endl;
    pool(thr_func1);
    std::cout << "check" << std::endl;
    f.join();
}

int fibio::main(int argc, char *argv[]) {
    fiber_group fg;
    fg.create_fiber(test_future);
    fg.create_fiber(test_async);
    fg.create_fiber(test_async_executor);
    fg.create_fiber(test_async_function);
    fg.create_fiber(test_wait_for_any1);
    fg.create_fiber(test_wait_for_any2);
    fg.create_fiber(test_wait_for_any3);
    fg.create_fiber(test_wait_for_all1);
    fg.create_fiber(test_wait_for_all2);
    fg.create_fiber(test_wait_for_all3);
    fg.create_fiber(test_async_wait_for_any1);
    fg.create_fiber(test_async_wait_for_any2);
    fg.create_fiber(test_async_wait_for_all1);
    fg.create_fiber(test_then1);
    fg.create_fiber(test_then2);
    fg.create_fiber(test_then3);
    fg.create_fiber(test_then4);
    fg.create_fiber(test_foreign_thread_pool);
    fg.join_all();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
