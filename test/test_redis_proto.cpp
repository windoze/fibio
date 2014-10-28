//
//  main.cpp
//  redis_proto
//
//  Created by Chen Xu on 14/9/24.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <iostream>
#include <sstream>
#include <fibio/redis/redis_proto.hpp>

using namespace fibio::redis;

bool check_io(const redis_data &d) {
    std::stringstream ss;
    ss << d;
    redis_data rd;
    ss >> rd;
    return d==rd;
}

void test_simple_string() {
    assert(check_io(simple_string()));
    assert(check_io(simple_string("this is a simple string")));
    assert(!check_io(simple_string("this is a simple string\r\nwith wrong format")));
}

void test_error() {
    assert(check_io(error()));
    assert(check_io(error("this is an error message")));
    assert(!check_io(error("this is an error message\r\nwith wrong format")));
}

void test_integer() {
    assert(check_io(0));
    assert(check_io(10));
    assert(check_io(-10));
    assert(check_io(10.5)); // This will be casted to int
}

void test_bulk_string() {
    assert(check_io(bulk_string()));
    assert(check_io(bulk_string("this is a bulk string")));
    assert(check_io(bulk_string("this is a bulk string\r\nwith CR/LF inside")));
}

void test_nil() {
    assert(check_io(nil));
}

void test_array() {
    assert(check_io(array()));
    assert(check_io(array{}));
    assert(check_io(array{2, simple_string("ss"), bulk_string("bulk"), 3}));
    assert(check_io(array{2, array{4,5}, 3}));
    assert(check_io(array{array{1,2}, 3, array{4,5}}));
    redis_data d(array{simple_string("str1"),
        error("err1"),
        57,
        bulk_string("blk1"),
        array{simple_string("str2"),
            error("err2"),
            nil,
            bulk_string("blk2"),
            68
        },
        nil,
        simple_string("str3"),
        error("err3"),
        79,
        bulk_string("blk3")});
    assert(check_io(d));
}

void test_data_type() {
    simple_string s("test");
    std::string s1("test");
    simple_string s2(s1);
    simple_string s3(std::move(s1));
    assert(s==s3);
    assert(s1.empty());
}

void test_set_cmd() {
    std::string cmd("*3\r\n$3\r\nset\r\n$2\r\naa\r\n$2\r\nbb\r\n123124");
    std::stringstream ss(cmd);
    redis_data d;
    ss >> d;
    std::cout << d;
    std::cout.flush();
}

int main(int argc, const char * argv[]) {
    test_simple_string();
    test_error();
    test_integer();
    test_bulk_string();
    test_nil();
    test_array();
    test_data_type();
    //test_set_cmd();
    return 0;
}
