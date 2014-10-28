//
//  client.cpp
//  fibio
//
//  Created by Chen Xu on 14/10/26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/lexical_cast.hpp>
#include <fibio/iostream.hpp>
#include <fibio/redis/client.hpp>

namespace fibio { namespace redis {
    namespace detail {
        template<typename T>
        boost::optional<T> make_optional(T t) {
            return (t==T()) ? boost::optional<T>() : boost::optional<T>(t);
        }
        template<typename T, typename U>
        boost::optional<T> make_optional(T t, U def) {
            return (t==def) ? boost::optional<T>() : boost::optional<T>(t);
        }
    }
    client::client(const std::string &host, const std::string &port) {
        stream_.connect(host, port);
    }
    
    client::client(const std::string &host, uint16_t port) {
        stream_.connect(host, boost::lexical_cast<std::string>(port));
    }
    
    void client::close() {
        stream_.close();
    }
    
    bool client::is_open() const {
        return stream_.is_open();
    }
    
    redis_data client::call(const array &d) {
        stream_ << d;
        redis_data ret;
        stream_ >> ret;
        return check_result(ret);
    }
    
    int64_t client::append(const std::string &key,
                           const std::string &value)
    {
        return call<int64_t>("APPEND", key, value);
    }
    
    bool client::auth(const std::string &password) {
        return call<bool>("AUTH", password);
    }
    
    nullable_result client::get(const std::string &key) {
        return call<nullable_result>("GET", key);
    }
    
    void client::bgrewriteaof() {
        call<void>("BGREWRITEAOF");
    }
    
    void client::bgsave() {
        call<void>("BGSAVE");
    }
    
    int64_t client::bitcount(const std::string &key) {
        return call<int64_t>("BITCOUNT", key);
    }
    
    int64_t client::bitcount(const std::string &key, std::pair<int64_t, int64_t> &&range) {
        return call<int64_t>("BITCOUNT", key, range);
    }

    int64_t client::bitpos(const std::string &key, bool bit, int64_t start, int64_t end) {
        return call<int64_t>("BITPOS",
                             key,
                             bit ? int64_t(1) : int64_t(0),
                             detail::make_optional(start),
                             detail::make_optional(end, std::numeric_limits<int64_t>::max()));
    }
    
    std::list<nullable_result> client::blpop(std::list<std::string> &&keys,
                                             int64_t timeout)
    {
        return call<std::list<nullable_result>>("BLPOP", keys, timeout);
    }
    
    std::list<nullable_result> client::brpop(std::list<std::string> &&keys,
                                             int64_t timeout)
    {
        return call<std::list<nullable_result>>("BRPOP", keys, timeout);
    }
    
    nullable_result client::brpoplpush(const std::string &source,
                                       const std::string &destination,
                                       int64_t timeout)
    {
        return call<nullable_result>("BRPOPLPUSH", source, destination, timeout);
    }
    
    std::string client::config_get(const std::string &parameter) {
        return call<bulk_string>("CONFIG", "GET", parameter);
    }
    
    void client::config_rewrite() {
        call<void>("CONFIG", "REWRITE");
    }
    
    void client::config_set(const std::string &key,
                            const std::string &value)
    {
        call<void>("CONFIG", "SET", key, value);
    }
    
    void client::config_resetstat() {
        call<void>("CONFIG", "RESETSTAT");
    }
    
    int64_t client::dbsize() {
        return call<int64_t>("DBSIZE");
    }
    
    std::string client::debug_object(const std::string &key) {
        return call<simple_string>("DEBUG", "OBJECT", key);
    }
    
    void client::debug_segfault() {
        call<void>("DEBUG", "SEGFAULT");
        close();
    }
    
    int64_t client::decr(const std::string &key) {
        return call<int64_t>("DECR", key);
    }
    
    int64_t client::decrby(const std::string &key, int64_t decrement) {
        return call<int64_t>("DECRBY", key, decrement);
    }
    
    void client::discard() {
        call<void>("DISCARD");
    }
    
    std::string client::dump(const std::string &key) {
        return call<bulk_string>("DUMP", key);
    }
    
    std::string client::echo(const std::string &message) {
        return call<bulk_string>("ECHO", message);
    }
    
    redis_data client::eval(const std::string &script,
                            std::list<std::string> &&keys,
                            std::list<std::string> &&args)
    {
        return call<redis_data>("EVAL", int64_t(keys.size()), keys, args);
    }
    
    redis_data client::evalsha(const std::string &sha1,
                               std::list<std::string> &&keys,
                               std::list<std::string> &&args)
    {
        return call<redis_data>("EVALSHA", int64_t(keys.size()), keys, args);
    }
    
    void client::exec() {
        call<void>("EXEC");
    }
    
    bool client::exists(const std::string &key) {
        return call<bool>("EXISTS", key);
    }
    
    bool client::expire(const std::string &key,
                        std::chrono::system_clock::duration expiration)
    {
        int64_t ms=std::chrono::duration_cast<std::chrono::microseconds>(expiration).count();
        if ((ms%1000)==0) {
            return call<bool>("EXPIRE", key, ms/1000);
        } else {
            return call<bool>("PEXPIRE", key, ms);
        }
    }
    
    bool client::expireat(const std::string &key,
                          std::chrono::system_clock::time_point expiration)
    {
        int64_t ms=std::chrono::duration_cast<std::chrono::microseconds>(expiration.time_since_epoch()).count();
        if ((ms%1000)==0) {
            return call<bool>("EXPIREAT", key, ms/1000);
        } else {
            return call<bool>("PEXPIREAT", key, ms);
        }
    }
    
    void client::flushall() {
        call<void>("FLUSHALL");
    }
    
    void client::flushdb() {
        call<void>("FLUSHDB");
    }
    
    bool client::getbit(const std::string &key, int64_t offset) {
        return call<bool>("GETBIT", key, offset);
    }
    
    std::string client::getrange(const std::string &key,
                                 int64_t start,
                                 int64_t end)
    {
        return call<bulk_string>("GETRANGE", key, start, end);
    }
    
    nullable_result client::getset(const std::string &key,
                                   const std::string &value)
    {
        return call<nullable_result>("GETSET", key, value);
    }
    
    int64_t client::hdel(const std::string &key, std::list<std::string> &&fields) {
        return call<int64_t>("HDEL", key, fields);
    }
    
    bool client::hexists(const std::string &key, const std::string &field) {
        return call<bool>("HEXISTS", key, field);
    }
    
    nullable_result client::hget(const std::string &key, const std::string &field) {
        return call<nullable_result>("HGET", key, field);
    }
    
    std::list<std::string> client::hgetall(const std::string &key) {
        return call<std::list<std::string>>("HGETALL", key);
    }
    
    int64_t client::hincrby(const std::string &key, const std::string &field, int64_t increment) {
        return call<int64_t>("HINCRBY", key, field, increment);
    }
    
    double client::hincrbyfloat(const std::string &key, const std::string &field, double increment) {
        return call<double>("HINCRBYFLOAT", key, field, increment);
    }
    
    std::list<std::string> client::hkeys(const std::string &key) {
        return call<std::list<std::string>>("HKEYS", key);
    }
    
    int64_t client::hlen(const std::string &key) {
        return call<int64_t>("HLEN", key);
    }
    
    std::list<nullable_result> client::hmget(const std::string &key, std::list<std::string> &&fields) {
        return call<std::list<nullable_result>>("HMGET", key, fields);
    }
    
    void client::hmset(const std::string &key, std::list<std::pair<std::string,std::string>> &&keys_values) {
        call<void>("HMSET", key, keys_values);
    }
    
    bool client::hset(const std::string &key, const std::string &field, const std::string &value) {
        return call<bool>("HSET", key, field, value);
    }
    
    bool client::hsetnx(const std::string &key, const std::string &field, const std::string &value) {
        return call<bool>("HSETNX", key, field, value);
    }
    
    std::list<std::string> client::hvals(const std::string &key) {
        return call<std::list<std::string>>("HVALS", key);
    }
    
    int64_t client::incr(const std::string &key) {
        return call<int64_t>("INCR", key);
    }
    
    int64_t client::incrby(const std::string &key, int64_t increment) {
        return call<int64_t>("INCRBY", key, increment);
    }
    
    double client::incrbyfloat(const std::string &key, const std::string &increment) {
        return call<double>("INCRBYFLOAT", key, increment);
    }
    
    std::string client::info(const std::string &section) {
        return call<bulk_string>("INFO", detail::make_optional(section));
    }
    
    std::chrono::system_clock::time_point client::lastsave() {
        return call<std::chrono::system_clock::time_point>("LASTSAVE");
    }
    
    nullable_result client::lindex(const std::string &key, int64_t index) {
        return call<nullable_result>("LINDEX", key, index);
    }
    
    int64_t client::linsert(const std::string &key, bool before, const std::string &pivot, const std::string &value) {
        array cmd=make_array("LINSERT", key);
        if (before) {
            cmd << "BEFORE";
        } else {
            cmd << "AFTER";
        }
        cmd << pivot << value;
        return extract<int64_t>(call(cmd));
    }
    
    int64_t client::llen(const std::string &key) {
        return call<int64_t>("LLEN", key);
    }
    
    nullable_result client::lpop(const std::string &key) {
        return call<nullable_result>("LPOP", key);
    }
    
    int64_t client::lpush(const std::string &key, std::list<std::string> &&values) {
        return call<int64_t>("LPUSH", key, values);
    }
    
    int64_t client::lpushx(const std::string &key, const std::string &value) {
        return call<int64_t>("LPUSHX", key, value);
    }
    
    std::list<std::string> client::lrange(const std::string &key, int64_t start, int64_t stop) {
        return call<std::list<std::string>>("LRANGE", key, start, stop);
    }
    
    int64_t client::lrem(const std::string &key, int64_t count, const std::string &value) {
        return call<int64_t>("LREM", key, count, value);
    }
    
    void client::lset(const std::string &key, int64_t index, const std::string &value) {
        call<void>("LSET", key, index, value);
    }
    
    void client::ltrim(const std::string &key, int64_t start, int64_t stop) {
        call<void>("LTRIM", key, start, stop);
    }
    
    std::list<nullable_result> client::mget(std::list<std::string> &&keys) {
        return call<std::list<nullable_result>>("MGET", keys);
    }
    
    void client::mset(std::list<std::pair<std::string,std::string>> &&keys_values) {
        call<void>("MSET", keys_values);
    }
    
    void client::msetnx(std::list<std::pair<std::string,std::string>> &&keys_values) {
        call<void>("MSETNX", keys_values);
    }
    
    bool client::persist(const std::string &key) {
        return call<bool>("PERSIST", key);
    }
    
    bool client::pfadd(const std::string &key, std::list<std::string> &&elements) {
        return call<bool>("PFADD", key, elements);
    }
    
    int64_t client::pfcount(std::list<std::string> &&keys) {
        return call<int64_t>("PFCOUNT", keys);
    }
    
    void client::pfmerge(const std::string &destkey, std::list<std::string> &&sourcekey) {
        call<void>("PFMERGE", destkey, sourcekey);
    }
    
    void client::ping() {
        call<void>("PING");
    }
    
    int64_t client::publish(const std::string &channel, const std::string &message) {
        return call<int64_t>("PUBLISH", channel, message);
    }
    
    void client::quit() {
        call<void>("QUIT");
        close();
    }
    
    std::string client::randomkey() {
        return call<bulk_string>("RANDOMKEY");
    }
    
    void client::rename(const std::string &key, const std::string &newkey) {
        call<void>("RENAME", key, newkey);
    }
    
    void client::renamenx(const std::string &key, const std::string &newkey) {
        call<void>("RENAMENX", key, newkey);
    }
    
    void client::restore(const std::string &key, std::chrono::system_clock::duration ttl, const std::string &serialized_value) {
        call<void>("RESTORE", ttl, serialized_value);
    }
    
    void client::restore(const std::string &key, const std::string &serialized_value) {
        restore(key, std::chrono::seconds(0), serialized_value);
    }
    
    array client::role() {
        return call<array>("ROLE");
    }
    
    nullable_result client::rpop(const std::string &key) {
        return call<nullable_result>("RPOP", key);
    }
    
    nullable_result client::rpoplpush(const std::string &source, const std::string &destination) {
        return call<nullable_result>("RPOPLPUSH", source, destination);
    }
    
    int64_t client::rpush(const std::string &key, std::list<std::string> &&values) {
        return call<int64_t>("RPUSH", key, values);
    }
    
    int64_t client::rpushx(const std::string &key, std::list<std::string> &&values) {
        return call<int64_t>("RPUSHX", key, values);
    }
    
    int64_t client::sadd(const std::string &key, std::list<std::string> &&members) {
        return call<int64_t>("SADD", key, members);
    }
    
    void client::save() {
        call<void>("SAVE");
    }
    
    int64_t client::scard(const std::string &key) {
        return call<int64_t>("SCARD", key);
    }
    
    std::list<bool> client::script_exists(std::list<std::string> &&scripts) {
        redis_data r=call(make_array("SCRIPT", "EXISTS", scripts));
        std::list<bool> ret;
        for(auto &i : boost::get<array>(r)) {
            ret.push_back(extract<bool>(std::move(i))==1);
        }
        return ret;
    }
    
    void client::script_flush() {
        call<void>("SCRIPT", "FLUSH");
    }
    
    void client::script_kill() {
        call<void>("SCRIPT", "KILL");
    }
    
    std::string client::script_load(const std::string &script) {
        return call<bulk_string>("SCRIPT", "KILL", script);
    }
    
    std::list<std::string> client::sdiff(std::list<std::string> &&keys) {
        return call<std::list<std::string>>("SDIFF", keys);
    }
    
    int64_t client::sdiffstore(const std::string &destination, std::list<std::string> &&keys) {
        return call<int64_t>("SDIFFSTORE", destination, keys);
    }
    
    void client::select(const std::string &index) {
        call<void>("SELECT", index);
    }
    
    bool client::set(const std::string &key,
                     const std::string &value,
                     std::chrono::system_clock::duration expiration,
                     bool nx,
                     bool xx)
    {
        array cmd=make_array("SET", key, value);
        if (expiration!=std::chrono::system_clock::duration()) {
            cmd << "EX" << expiration;
        }
        if (nx) {
            cmd << "NX";
        }
        if (xx) {
            cmd << "XX";
        }
        return data_type(call(cmd))==SIMPLE_STRING;
    }
    
    bool client::setbit(const std::string &key, int64_t offset, bool value) {
        return call<bool>("SETBITS", key, offset, int64_t(value));
    }
    
    int64_t client::setrange(const std::string &key, int64_t offset, const std::string &value) {
        return call<int64_t>("SETRANGE", key, offset, value);
    }
    
    void client::shutdown(bool nosave, bool save) {
        array cmd=make_array("SHUTDOWN");
        if (nosave) {
            cmd << "NOSAVE";
        }
        if (save) {
            cmd << "SAVE";
        }
        call(cmd);
        close();
    }
    
    std::list<std::string> client::sinter(std::list<std::string> &&keys) {
        return call<std::list<std::string>>("SINTER", keys);
    }

    int64_t client::sinterstore(const std::string &destination, std::list<std::string> &&keys) {
        return call<int64_t>("SINTERSTORE", destination, keys);
    }
    
    bool client::sismember(const std::string &key, const std::string &member) {
        return call<bool>("SISMEMBER", key, member);
    }
    
    void client::slaveof(const std::string &host, uint16_t port) {
        call<void>("SLAVEOF", host, int64_t(port));
    }
    
    std::list<std::string> client::smembers(const std::string &key) {
        return call<std::list<std::string>>("SMEMBERS", key);
    }
    
    bool client::smove(const std::string &source, const std::string &destination, const std::string &member) {
        return call<bool>("SMOVE", source, destination, member);
    }
    
    nullable_result client::spop(const std::string &key) {
        return call<nullable_result>("SPOP", key);
    }
    
    nullable_result client::srandmember(const std::string &key) {
        return call<nullable_result>("SRANDMEMBER", key);
    }
    
    std::list<std::string> client::srandmember(const std::string &key, int64_t count) {
        return call<std::list<std::string>>("SRANDMEMBER", key, count);
    }
    
    int64_t client::srem(const std::string &key, std::list<std::string> &&members) {
        return call<int64_t>("SREM", key, members);
    }
    
    int64_t client::strlen(const std::string &key) {
        return call<int64_t>("STRLEN", key);
    }
    
    std::list<std::string> client::sunion(std::list<std::string> &&keys) {
        return call<std::list<std::string>>("SUNION", keys);
    }
    
    int64_t client::sunionstore(const std::string &destination, std::list<std::string> &&keys) {
        return call<int64_t>("SUNIONSTORE", destination, keys);
    }
    
    void client::sync() {
        call<void>("SYNC");
    }
    
    std::chrono::system_clock::time_point client::time() {
        return call<std::chrono::system_clock::time_point>("TIME");
    }
    
    std::string client::type(const std::string &key) {
        return call<simple_string>("TYPE", key);
    }

    int64_t client::zadd(const std::string &key, std::list<std::pair<double, std::string>> &&scores_members) {
        return call<int64_t>("ZADD", key, scores_members);
    }
    
    int64_t client::zcard(const std::string &key) {
        return call<int64_t>("ZCARD", key);
    }
    
    int64_t client::zcount(const std::string &key, int64_t min_, int64_t max_) {
        return call<int64_t>("ZCOUNT", key, min_, max_);
    }
    
    double client::zincrby(const std::string &key, double increment, const std::string &member) {
        return call<double>("ZINCRBY", key, increment, member);
    }

    int64_t client::zlexcount(const std::string &key, const std::string &min_, const std::string &max_) {
        return call<int64_t>("ZLEXCOUNT", key, min_, max_);
    }
    
    std::list<std::string> client::zrange(const std::string &key, int64_t start, int64_t stop) {
        return call<std::list<std::string>>("ZRANGE", key, start, stop);
    }
    
    std::list<std::pair<std::string, int64_t>> client::zrange_withscores(const std::string &key, int64_t start, int64_t stop) {
        redis_data r=call(make_array("ZRANGE", key, start, stop, "WITHSCORES"));
        array &a=boost::get<array>(r);
        std::list<std::pair<std::string, int64_t>> ret;
        auto i=a.begin();
        while (i!=a.end()) {
            std::pair<std::string, int64_t> p;
            p.first.assign(std::move(boost::get<bulk_string>(*i)));
            ++i;
            p.second=extract<double>(std::move(*i));
            ++i;
            ret.push_back(std::move(p));
        }
        return ret;
    }

    std::list<std::string> client::zrangebylex(const std::string &key, const std::string &min_, const std::string &max_) {
        return call<std::list<std::string>>("ZRANGEBYLEX", key, min_, max_);
    }
    
    std::list<std::string> client::zrangebylex(const std::string &key, const std::string &min_, const std::string &max_, int64_t offset, int64_t count) {
        return call<std::list<std::string>>("ZRANGEBYLEX", key, min_, max_, "LIMIT", offset, count);
        
    }
}}  // End of namespace fibio::redis
















