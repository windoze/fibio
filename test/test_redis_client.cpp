//
//  test_redis_client.cpp
//  fibio
//
//  Created by Chen Xu on 14/10/26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <stdio.h>
#include <fibio/redis/client.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio::redis;

void test_get_set(client &c) {
    c.flushdb();
    c.set("k1", "v1");
    assert(*c.get("k1")=="v1");
    c.set("k1", "v2");
    assert(*c.get("k1")=="v2");
    assert(!c.get("nonexist"));
    
    c.set("mykey", "10.5");
    c.incrbyfloat("mykey", "0.1");
    assert(*(c.get("mykey"))=="10.6");
}

void test_bitcount(client &c) {
    c.flushdb();
    c.set("mykey", "foobar");
    assert(c.bitcount("mykey")==26);
    assert(c.bitcount("mykey", {0,0})==4);
    assert(c.bitcount("mykey", {1,1})==6);
    assert(c.bitcount("nonexist", {8,12})==0);
}

void test_sort(client &c) {
    c.flushdb();
    c.lpush("mylist", {"c"});
    c.lpush("mylist", {"b", "a"});
    client::sort_criteria crit;
    crit.alpha=true;
    c.sort("mylist", crit);
    assert((c.lrange("mylist", 0, 100)==std::list<std::string>{"a", "b", "c"}));
}

int fibio::main(int argc, char *argv[]) {
    popen("PATH=/usr/bin:/usr/local/bin redis-server --port 37777", "r");
    this_fiber::sleep_for(std::chrono::seconds(1));
    client c("127.0.0.1", 37777);
    
    test_get_set(c);
    test_bitcount(c);
    test_sort(c);
    
    c.shutdown();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
