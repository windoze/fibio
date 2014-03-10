//
//  main.cpp
//  fibio
//
//  Created by Chen Xu on 14-2-20.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>
#include <fibio/fiber.hpp>
#include <fibio/concurrent/concurrent_queue.hpp>
#include <fibio/io/io.hpp>
#include <fibio/stream/iostream.hpp>
#include <fibio/fibers/fiberize.hpp>

using namespace fibio;

mutex m;
condition_variable cv;
concurrent::concurrent_queue<int> q;
fibio::fiber_specific_ptr<int> int_fss;

void f(int n) {
#if 1
    int_fss.reset(new int(n));
    {
        lock_guard<mutex> lock(m);
        std::cout << "fiber " << n << " sees fss is " << *int_fss << std::endl;
    }
    for (int i=0; i<100; i++) {
        {
            lock_guard<mutex> lock(m);
            //std::cout << "fiber " << n << ": " << i << std::endl;
        }
        this_fiber::yield();
    }
    {
        lock_guard<mutex> lock(m);
        std::cout << "fiber " << n << " sees fss is " << *int_fss << std::endl;
    }
#endif
}

void child() {
    try {
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber child started" << std::endl;
        }
        for (int i=0; ; i++) {
            lock_guard<mutex> lock(m);
            if(!q.push(i))
                break;
            std::cout << "fiber child: sent " << i << std::endl;
        }
        q.close();
        this_fiber::sleep_for(std::chrono::seconds(3));
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber child: notify one" << std::endl;
        }
        cv.notify_one();
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber child exited" << std::endl;
        }
    } catch(std::system_error e) {
        std::cout << "fiber child: exception " << e.std::exception::what() << std::endl;
    }
}

void parent() {
    try {
        {
            lock_guard<mutex> lock(m);
            if (1) {
                try {
                    lock_guard<mutex> lock(m);
                } catch(std::system_error e) {
                    std::cout << "fiber parent: will deadlock" << std::endl;
                }
            }
            std::cout << "fiber parent started" << std::endl;
        }
        fibio::fiber f(child);
        for (auto &i : q) {
            lock_guard<mutex> lock(m);
            std::cout << "fiber parent: got " << i << std::endl;
        }
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber parent: waiting for cv" << std::endl;
        }
        if(1)
        {
            unique_lock<mutex> lock(m);
            cv_status r=cv.wait_for(lock, std::chrono::seconds(1));
            if (r==cv_status::timeout) {
                std::cout << "fiber parent: cv timeout" << std::endl;
            } else {
                std::cout << "fiber parent: cv notified" << std::endl;
            }
        } else {
            unique_lock<mutex> lock(m);
            cv.wait(lock);
            std::cout << "fiber parent: cv notified" << std::endl;
        }
        f.join();
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber parent exited" << std::endl;
        }
    } catch(std::system_error e) {
        std::cout << "fiber parent: exception " << e.std::exception::what() << std::endl;
    }
}

void test_connect() {
    try {
        {
            lock_guard<mutex> lock(m);
            std::cout << "fiber test_connect started" << std::endl;
        }
        fibio::stream::tcp_stream str;
        str.set_connect_timeout(std::chrono::milliseconds(1000));
        if(!str.connect("0d0a.com", "80")) {
            {
                lock_guard<mutex> lock(m);
                std::cout << "fiber test_connect: connected" << std::endl;
            }
            const char *data="GET / HTTP/1.0\r\nHost:0d0a.com\r\n";
            char recv[1025];
            memset(recv, 0, 1025);
            str << data << std::endl;
            {
                lock_guard<mutex> lock(m);
                std::cout << "fiber test_connect: request sent" << std::endl;
            }
            str.set_read_timeout(std::chrono::milliseconds(1000));
            str.readsome(recv, 1024);
            {
                lock_guard<mutex> lock(m);
                std::cout << "fiber test_connect " << std::endl;
                std::cout << recv << std::endl;
            }
        } else {
            lock_guard<mutex> lock(m);
            std::cout << "fiber test_connect: connect timeout" << std::endl;
        }
        q.close();
    } catch(std::runtime_error &e) {
        lock_guard<mutex> lock(m);
        std::cout << "fiber test_connect: exception " << e.std::exception::what() << std::endl;
    }
}

timed_mutex m1;

void child1() {
    {
        std::cout << "fiber child1 started" << std::endl;
        lock_guard<timed_mutex> lock(m1);
        std::cout << "fiber child1: locked m1" << std::endl;
        this_fiber::sleep_for(std::chrono::seconds(3));
    }
    std::cout << "fiber child1: released m1" << std::endl;
}

void parent1() {
    std::cout << "fiber parent1 started" << std::endl;
    fibio::fiber f(child1);
    this_fiber::sleep_for(std::chrono::seconds(1));
    std::cout << "fiber parent1: started child1" << std::endl;
    {
        std::cout << "fiber parent1: try to lock m1" << std::endl;
        if (m1.try_lock_for(std::chrono::seconds(1))) {
            std::cout << "fiber parent1: m1 acquired" << std::endl;
            m1.unlock();
            std::cout << "fiber parent1: m1 released" << std::endl;
        } else {
            std::cout << "fiber parent1: m1 timeout" << std::endl;
        }
    }
    f.join();
}

int main_fiber(int argc, char *argv[])
{
    // std::{cin,cout,cerr} is fiberized
    // std::cout is buffered
    std::cout << "Hello, world1\n";
    std::cout << "Hello, world2" << std::endl;
    
    // std::cerr is unbuffered
    std::cerr << "Hello, world3\n";
    std::cerr << "Hello, world4" << std::endl;

    int_fss.reset(new int(1000));
    std::cout << "fiber main sees fss is " << *int_fss << std::endl;
    
    std::vector<fiber> fibers;
    for (int i=0; i<30; i++) {
        //fibers.push_back(fiber([i](){ f(i); }));
        fiber fb([i](){ f(i); });
        fb.detach();
    }
    
    //fibers.push_back(fiber(parent));
    //fibers.push_back(fiber(parent1));
    //fibers.push_back(fiber(test_connect));

    for (fiber &f : fibers) {
        f.join();
    }
    
    //this_fiber::sleep_for(std::chrono::seconds(3));
    {
        lock_guard<mutex> lock(m);
        std::cout << "fiber main sees fss is " << *int_fss << std::endl;
        std::cout << "main fiber exited" << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    fibio::fibers::fiberize(3, main_fiber, argc, argv);
    return 0;
}

struct STD_COUT_TEST{
    ~STD_COUT_TEST() {
        // std::cout still works after fiberized main exists
        std::cout << "std::cout still works after fiberized main exists" << std::endl;
    }
} std_cout_test;
