//
//  test_redis_client.cpp
//  fibio
//
//  Created by Chen Xu on 14/10/26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <set>
#include <fibio/redis/client.hpp>
#include <fibio/fiberize.hpp>

using namespace fibio;
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

void test_pub() {
    client c("127.0.0.1:37777");
    this_fiber::sleep_for(std::chrono::seconds(1));
    c.publish("abc", "123");
    c.publish("def", "456");
}

void test_sub(client &c) {
    message_queue &q=c.subscribe({"abc", "def"});
    fiber pubber(test_pub);
    redis_message msg;
    q.pop(msg);
    assert(msg.first=="abc");
    assert(msg.second=="123");
    q.pop(msg);
    assert(msg.first=="def");
    assert(msg.second=="456");
    c.unsubscribe({"abc", "def"});
    this_fiber::sleep_for(std::chrono::seconds(1));
    assert(!q.is_open());
    pubber.join();
}

void test_scan(client &c) {
    c.flushdb();
    std::set<std::string> expected;
    for (int i=42; i<442; i++) {
        std::string k("key");
        k+=boost::lexical_cast<std::string>(i);
        std::string v("key");
        v+=boost::lexical_cast<std::string>(i);
        c.set(k, v);
        expected.insert(k);
    }
    
    std::set<std::string> keys;
    for(auto i=c.scan(); i!=c.end(); ++i) {
        keys.insert(*i);
    }
    assert(keys==expected);
}

void test_sscan(client &c) {
    c.flushdb();
    std::set<std::string> expected;
    for (int i=42; i<442; i++) {
        std::string k("key");
        k+=boost::lexical_cast<std::string>(i);
        c.sadd("myset", {k});
        expected.insert(k);
    }
    
    std::set<std::string> keys;
    for(auto i=c.sscan("myset"); i!=c.end(); ++i) {
        keys.insert(*i);
    }
    assert(keys==expected);
}

void test_hscan(client &c) {
    c.flushdb();
    std::set<std::string> expected;
    for (int i=42; i<442; i++) {
        std::string k("key");
        k+=boost::lexical_cast<std::string>(i);
        std::string v("key");
        v+=boost::lexical_cast<std::string>(i);
        c.hset("myhash", k, v);
        expected.insert(k);
    }
    
    std::set<std::string> keys;
    for(auto i=c.hscan("myhash"); i!=c.end(); ++i) {
        keys.insert(*i);
    }
    assert(keys==expected);
}

void test_zscan(client &c) {
    c.flushdb();
    std::set<std::string> expected;
    for (int i=42; i<442; i++) {
        std::string k("key");
        k+=boost::lexical_cast<std::string>(i);
        c.zadd("myzset", {{1.2, k}});
        expected.insert(k);
    }
    
    std::set<std::string> keys;
    for(auto i=c.zscan("myzset"); i!=c.end(); ++i) {
        keys.insert(*i);
        assert(i.score()==1.2);
    }
    assert(keys==expected);
}

int fibio::main(int argc, char *argv[]) {
    popen("PATH=/usr/bin:/usr/local/bin redis-server --port 37777", "r");
    this_fiber::sleep_for(std::chrono::seconds(1));
    client c("127.0.0.1:37777");
    
    test_get_set(c);
    test_bitcount(c);
    test_sort(c);
    test_sub(c);
    test_scan(c);
    test_sscan(c);
    test_hscan(c);
    test_zscan(c);
    
    c.shutdown();
    std::cout << "main_fiber exiting" << std::endl;
    return 0;
}
