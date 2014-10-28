//
//  client.hpp
//  fibio
//
//  Created by Chen Xu on 14/10/26.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_redis_client_hpp
#define fibio_redis_client_hpp

#include <chrono>
#include <boost/optional.hpp>
#include <fibio/iostream.hpp>
#include <fibio/redis/redis_proto.hpp>

namespace fibio { namespace redis {
    namespace detail {
        
    }
    
    constexpr uint16_t DEFAULT_REDIS_PORT=6379;
    struct client {
        client(const std::string &host, const std::string &port);
        client(const std::string &host, uint16_t port=DEFAULT_REDIS_PORT);
        client(const std::string &host_port);
        
        void close();
        bool is_open() const;

        int64_t append(const std::string &key,
                       const std::string &value);
        bool auth(const std::string &password);
        void bgrewriteaof();
        void bgsave();
        int64_t bitcount(const std::string &key);
        int64_t bitcount(const std::string &key,
                         std::pair<int64_t, int64_t> &&range);
        int64_t bitpos(const std::string &key,
                       bool bit,
                       int64_t start=0,
                       int64_t end=std::numeric_limits<int64_t>::max());
        std::list<nullable_result> blpop(std::list<std::string> &&keys,
                                         int64_t timeout);
        std::list<nullable_result> brpop(std::list<std::string> &&keys,
                                         int64_t timeout);
        nullable_result brpoplpush(const std::string &source,
                                   const std::string &destination,
                                   int64_t timeout);
        std::string config_get(const std::string &parameter);
        void config_rewrite();
        void config_set(const std::string &key,
                        const std::string &value);
        void config_resetstat();
        int64_t dbsize();
        std::string debug_object(const std::string &key);
        void debug_segfault();
        int64_t decr(const std::string &key);
        int64_t decrby(const std::string &key,
                       int64_t decrement);
        void discard();
        std::string dump(const std::string &key);
        std::string echo(const std::string &message);
        redis_data eval(const std::string &script,
                        std::list<std::string> &&keys,
                        std::list<std::string> &&args);
        redis_data evalsha(const std::string &script,
                           std::list<std::string> &&keys,
                           std::list<std::string> &&args);
        void exec();
        bool exists(const std::string &key);
        bool expire(const std::string &key,
                    std::chrono::system_clock::duration expiration);
        bool expireat(const std::string &key,
                      std::chrono::system_clock::time_point expiration);
        void flushall();
        void flushdb();
        nullable_result get(const std::string &key);
        bool getbit(const std::string &key,
                    int64_t offset);
        std::string getrange(const std::string &key,
                             int64_t start,
                             int64_t end);
        nullable_result getset(const std::string &key,
                               const std::string &value);
        int64_t hdel(const std::string &key,
                     std::list<std::string> &&fields);
        bool hexists(const std::string &key,
                     const std::string &field);
        nullable_result hget(const std::string &key,
                             const std::string &field);
        std::list<std::string> hgetall(const std::string &key);
        int64_t hincrby(const std::string &key,
                        const std::string &field,
                        int64_t increment);
        double hincrbyfloat(const std::string &key,
                            const std::string &field,
                            double increment);
        std::list<std::string> hkeys(const std::string &key);
        int64_t hlen(const std::string &key);
        std::list<nullable_result> hmget(const std::string &key,
                                         std::list<std::string> &&fields);
        void hmset(const std::string &key,
                   std::list<std::pair<std::string,std::string>> &&keys_values);
        bool hset(const std::string &key,
                  const std::string &field,
                  const std::string &value);
        bool hsetnx(const std::string &key,
                    const std::string &field,
                    const std::string &value);
        std::list<std::string> hvals(const std::string &key);
        int64_t incr(const std::string &key);
        int64_t incrby(const std::string &key,
                       int64_t increment);
        double incrbyfloat(const std::string &key,
                           const std::string &increment);
        std::string info(const std::string &section="");
        std::chrono::system_clock::time_point lastsave();
        nullable_result lindex(const std::string &key,
                               int64_t index);
        int64_t linsert(const std::string &key,
                        bool before,
                        const std::string &pivot,
                        const std::string &value);
        int64_t llen(const std::string &key);
        nullable_result lpop(const std::string &key);
        int64_t lpush(const std::string &key,
                      std::list<std::string> &&values);
        int64_t lpushx(const std::string &key,
                       const std::string &value);
        std::list<std::string> lrange(const std::string &key,
                                      int64_t start,
                                      int64_t stop);
        int64_t lrem(const std::string &key,
                     int64_t count,
                     const std::string &value);
        void lset(const std::string &key,
                  int64_t index,
                  const std::string &value);
        void ltrim(const std::string &key,
                   int64_t start,
                   int64_t stop);
        std::list<nullable_result> mget(std::list<std::string> &&keys);
        
        // MIGRATE host port key destination-db timeout [COPY] [REPLACE]
        // void monitor();
        
        void mset(std::list<std::pair<std::string,std::string>> &&keys_values);
        void msetnx(std::list<std::pair<std::string,std::string>> &&keys_values);
        
        // void multi();
        // void object(const std::string &subcommand, std::list<std::string> &&arguments);
        
        bool persist(const std::string &key);
        
        bool pfadd(const std::string &key,
                   std::list<std::string> &&elements);
        int64_t pfcount(std::list<std::string> &&keys);
        void pfmerge(const std::string &destkey,
                     std::list<std::string> &&sourcekey);
        // NOTE: Use expire: bool pexpire(const std::string &key, std::chrono::system_clock::duration expiration);
        // NOTE: Use expireat: bool pexpireat(const std::string &key, std::chrono::system_clock::time_point expiration);
        void ping();
        // PSUBSCRIBE pattern [pattern ...]
        // PUBSUB subcommand [argument [argument ...]]

        // std::chrono::system_clock::duration pttl(const std::string &key);
        
        int64_t publish(const std::string &channel, const std::string &message);
    
        // TODO: PUNSUBSCRIBE [pattern [pattern ...]]
        
        void quit();
        std::string randomkey();
        void rename(const std::string &key,
                    const std::string &newkey);
        void renamenx(const std::string &key,
                      const std::string &newkey);
        void restore(const std::string &key,
                     std::chrono::system_clock::duration ttl,
                     const std::string &serialized_value);
        void restore(const std::string &key,
                     const std::string &serialized_value);
        array role();
        nullable_result rpop(const std::string &key);
        nullable_result rpoplpush(const std::string &source,
                                  const std::string &destination);
        int64_t rpush(const std::string &key,
                      std::list<std::string> &&values);
        int64_t rpushx(const std::string &key,
                       std::list<std::string> &&values);
        int64_t sadd(const std::string &key,
                     std::list<std::string> &&members);
        void save();
        int64_t scard(const std::string &key);
        std::list<bool> script_exists(std::list<std::string> &&scripts);
        void script_flush();
        void script_kill();
        std::string script_load(const std::string &script);
        std::list<std::string> sdiff(std::list<std::string> &&keys);
        int64_t sdiffstore(const std::string &destination,
                           std::list<std::string> &&keys);
        void select(const std::string &index);
        bool set(const std::string &key,
                 const std::string &value,
                 std::chrono::system_clock::duration expiration=std::chrono::system_clock::duration(),
                 bool nx=false,
                 bool xx=false);
        bool setbit(const std::string &key,
                    int64_t offset,
                    bool value);
        // NOTE: Use set, SETEX key seconds value
        // NOTE: Use set, SETNX key value
        int64_t setrange(const std::string &key,
                         int64_t offset,
                         const std::string &value);
        void shutdown(bool nosave=false, bool save=false);
        std::list<std::string> sinter(std::list<std::string> &&keys);
        int64_t sinterstore(const std::string &destination,
                            std::list<std::string> &&keys);
        bool sismember(const std::string &key,
                       const std::string &member);
        void slaveof(const std::string &host,
                     uint16_t port);
        // SLOWLOG subcommand [argument]
        std::list<std::string> smembers(const std::string &key);
        bool smove(const std::string &source,
                   const std::string &destination,
                   const std::string &member);
        // SORT key [BY pattern] [LIMIT offset count] [GET pattern [GET pattern ...]] [ASC|DESC] [ALPHA] [STORE destination]
        nullable_result spop(const std::string &key);
        nullable_result srandmember(const std::string &key);
        std::list<std::string> srandmember(const std::string &key,
                                           int64_t count);
        int64_t srem(const std::string &key,
                     std::list<std::string> &&members);
        int64_t strlen(const std::string &key);
        // SUBSCRIBE channel [channel ...]
        std::list<std::string> sunion(std::list<std::string> &&keys);
        int64_t sunionstore(const std::string &destination,
                            std::list<std::string> &&keys);
        void sync();
        std::chrono::system_clock::time_point time();
        // std::chrono::system_clock::duration ttl(const std::string &key);
        std::string type(const std::string &key);
        // UNSUBSCRIBE [channel [channel ...]]
        // WATCH key [key ...]
        
        int64_t zadd(const std::string &key,
                     std::list<std::pair<double, std::string>> &&scores_members);
        int64_t zcard(const std::string &key);
        int64_t zcount(const std::string &key,
                       int64_t min_,
                       int64_t max_);
        double zincrby(const std::string &key,
                       double increment,
                       const std::string &member);
        // ZINTERSTORE destination numkeys key [key ...] [WEIGHTS weight [weight ...]] [AGGREGATE SUM|MIN|MAX]
        int64_t zlexcount(const std::string &key,
                          const std::string &min_,
                          const std::string &max_);
        std::list<std::string> zrange(const std::string &key,
                                      int64_t start,
                                      int64_t stop);
        std::list<std::pair<std::string, int64_t>> zrange_withscores(const std::string &key,
                                                                     int64_t start,
                                                                     int64_t stop);
        std::list<std::string> zrangebylex(const std::string &key,
                                           const std::string &min_,
                                           const std::string &max_);
        std::list<std::string> zrangebylex(const std::string &key,
                                           const std::string &min_,
                                           const std::string &max_,
                                           int64_t offset,
                                           int64_t count);
        // ZRANGEBYSCORE key min max [WITHSCORES] [LIMIT offset count]
        // ZRANK key member
        // ZREM key member [member ...]
        // ZREMRANGEBYLEX key min max
        // ZREMRANGEBYRANK key start stop
        // ZREMRANGEBYSCORE key min max
        // ZREVRANGE key start stop [WITHSCORES]
        // ZREVRANGEBYSCORE key max min [WITHSCORES] [LIMIT offset count]
        // ZREVRANK key member
        // ZSCORE key member
        // ZUNIONSTORE destination numkeys key [key ...] [WEIGHTS weight [weight ...]] [AGGREGATE SUM|MIN|MAX]

        // SCAN cursor [MATCH pattern] [COUNT count]
        // SSCAN key cursor [MATCH pattern] [COUNT count]
        // HSCAN key cursor [MATCH pattern] [COUNT count]
        // ZSCAN key cursor [MATCH pattern] [COUNT count]
        
        template<typename R, typename... Args>
        R call(Args&&... args) {
            return extract<R>(call(make_array(std::forward<Args>(args)...)));
        }
        template<typename R>
        R call(const array &cmd) {
            return extract<R>(call(cmd));
        }
        redis_data call(const array &cmd);
    private:
        tcp_stream stream_;
    };
}}  // End of namespace fibio::redis

#endif
