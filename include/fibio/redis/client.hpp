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
#include <fibio/concurrent/concurrent_queue.hpp>

namespace fibio { namespace redis {
    enum aggregate_type {
        NO_AGGREGATE=0,
        SUM=1,
        MIN=2,
        MAX=3,
    };
    
    typedef std::pair<std::string, std::string> redis_message;
    typedef concurrent::concurrent_queue<redis_message> message_queue;
    
    struct client {
        client(const std::string &host_port);
        client(const std::string &host, uint16_t port);
        
        void close();
        bool is_open() const;
        
#if 0
        // TODO: Transaction
        void discard();
        void exec();
        void multi();
        void unwatch();
        void watch(std::list<std::string> &&keys);
#endif
        
        // Pub/Sub
        message_queue &psubscribe(std::list<std::string> &&patterns);
        message_queue &subscribe(std::list<std::string> &&channels);
        void punsubscribe(std::list<std::string> &&patterns);
        void unsubscribe(std::list<std::string> &&channels);
        bool subscribing() const;
        
        // Scanning
        struct scan_iterator : std::iterator<std::input_iterator_tag, std::string> {
            typedef const std::string &const_reference;
            
            scan_iterator()=default;
            scan_iterator(client *cl, array &&scan_cmd, size_t cur_pos);
            bool operator==(const scan_iterator &other) const;
            bool operator!=(const scan_iterator &other) const;
            reference operator*();
            const_reference operator*() const;
            const_reference operator->() const;
            scan_iterator &operator++();

        private:
            bool ended() const;
            void next_batch();
            void next();
            client *c=nullptr;
            int64_t cursor=0;
            array current_batch;
            bool last_batch=false;
            array::iterator cur;
            array cmd;
            array::iterator cursor_pos;
            friend struct client;
        };
        scan_iterator end() const { return scan_iterator(); }
        
        scan_iterator scan();
        scan_iterator scan(const std::string &pattern);
        scan_iterator scan(size_t count);
        scan_iterator scan(const std::string &pattern, size_t count);
        
        scan_iterator sscan(const std::string &key);
        scan_iterator sscan(const std::string &key, const std::string &pattern);
        scan_iterator sscan(const std::string &key, size_t count);
        scan_iterator sscan(const std::string &key, const std::string &pattern, size_t count);
        
        scan_iterator hscan(const std::string &key);
        scan_iterator hscan(const std::string &key, const std::string &pattern);
        scan_iterator hscan(const std::string &key, size_t count);
        scan_iterator hscan(const std::string &key, const std::string &pattern, size_t count);
        
        struct zscan_iterator : scan_iterator {
            using scan_iterator::scan_iterator;
            // HACK: zscan returns key/score pair, use score() to retrieve score
            zscan_iterator &operator++();
            double score() const;
        };
        zscan_iterator zscan(const std::string &key);
        zscan_iterator zscan(const std::string &key, const std::string &pattern);
        zscan_iterator zscan(const std::string &key, size_t count);
        zscan_iterator zscan(const std::string &key, const std::string &pattern, size_t count);
        
        // Sorting
        struct sort_criteria {
            std::string by_pattern;
            int64_t offset=0;
            int64_t count=-1;
            std::list<std::string> get_patterns;
            bool asc=true;
            bool alpha=false;
            std::string destination;
        };
        std::list<std::string> sort(const std::string &key, sort_criteria &&crit);
        std::list<std::string> sort(const std::string &key, const sort_criteria &crit) {
            return sort(key, sort_criteria(crit));
        }
        
        // Commands
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
        std::string dump(const std::string &key);
        std::string echo(const std::string &message);
        redis_data eval(const std::string &script,
                        std::list<std::string> &&keys,
                        std::list<std::string> &&args);
        redis_data evalsha(const std::string &script,
                           std::list<std::string> &&keys,
                           std::list<std::string> &&args);
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
        void migrate(const std::string &host,
                     uint16_t port,
                     const std::string &key,
                     const std::string &destination_db,
                     std::chrono::system_clock::duration timeout,
                     bool copy=false,
                     bool replace=false);
        stream::closable_stream &monitor();
        
        void mset(std::list<std::pair<std::string,std::string>> &&keys_values);
        void msetnx(std::list<std::pair<std::string,std::string>> &&keys_values);
        
        std::string object_encoding(const std::string &key);
        int64_t object_idletime(const std::string &key);
        int64_t object_refcount(const std::string &key);
        
        bool persist(const std::string &key);
        
        bool pfadd(const std::string &key,
                   std::list<std::string> &&elements);
        int64_t pfcount(std::list<std::string> &&keys);
        void pfmerge(const std::string &destkey,
                     std::list<std::string> &&sourcekey);
        // NOTE: Use expire: bool pexpire(const std::string &key, std::chrono::system_clock::duration expiration);
        // NOTE: Use expireat: bool pexpireat(const std::string &key, std::chrono::system_clock::time_point expiration);
        void ping();
        std::list<std::string> pubsub_channel();
        std::list<std::string> pubsub_channel(const std::string &pattern);
        std::list<std::pair<std::string, int64_t>> pubsub_numsub(std::list<std::string> &&channels);
        int64_t pubsub_numpat();
        // NOTE: Uses ttl, std::chrono::system_clock::duration pttl(const std::string &key);
        int64_t publish(const std::string &channel, const std::string &message);
        
        
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
        array slowlog_get(int64_t n);
        int64_t slowlog_len();
        void slowlog_reset();
        std::list<std::string> smembers(const std::string &key);
        bool smove(const std::string &source,
                   const std::string &destination,
                   const std::string &member);
        nullable_result spop(const std::string &key);
        nullable_result srandmember(const std::string &key);
        std::list<std::string> srandmember(const std::string &key,
                                           int64_t count);
        int64_t srem(const std::string &key,
                     std::list<std::string> &&members);
        int64_t strlen(const std::string &key);
        std::list<std::string> sunion(std::list<std::string> &&keys);
        int64_t sunionstore(const std::string &destination,
                            std::list<std::string> &&keys);
        void sync();
        std::chrono::system_clock::time_point time();
        std::chrono::system_clock::duration ttl(const std::string &key);
        std::string type(const std::string &key);
        int64_t zadd(const std::string &key,
                     std::list<std::pair<double, std::string>> &&scores_members);
        int64_t zcard(const std::string &key);
        int64_t zcount(const std::string &key,
                       int64_t min_,
                       int64_t max_);
        double zincrby(const std::string &key,
                       double increment,
                       const std::string &member);
        int64_t zinterstore(const std::string &destination,
                            std::list<std::string> &&keys,
                            aggregate_type agg=NO_AGGREGATE);
        int64_t zinterstore(const std::string &destination,
                            std::list<std::string> &&keys,
                            std::list<double> &&weights,
                            aggregate_type agg=NO_AGGREGATE);
        int64_t zlexcount(const std::string &key,
                          const std::string &min_,
                          const std::string &max_);
        std::list<std::string> zrange(const std::string &key,
                                      int64_t start,
                                      int64_t stop);
        std::list<std::pair<std::string, double>> zrange_withscores(const std::string &key,
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
        std::list<std::string> zrangebyscore(const std::string &key,
                                             const std::string &min_,
                                             const std::string &max_);
        std::list<std::string> zrangebyscore(const std::string &key,
                                             const std::string &min_,
                                             const std::string &max_,
                                             int64_t offset,
                                             int64_t count);
        std::list<std::pair<std::string, double>> zrangebyscore_withscores(const std::string &key,
                                                                           const std::string &min_,
                                                                           const std::string &max_);
        std::list<std::pair<std::string, double>> zrangebyscore_withscores(const std::string &key,
                                                                           const std::string &min_,
                                                                           const std::string &max_,
                                                                           int64_t offset,
                                                                           int64_t count);
        boost::optional<int64_t> zrank(const std::string &key, const std::string &member);
        int64_t zrem(const std::string &key, std::list<std::string> &&members);
        int64_t zremrangebylex(const std::string &key,
                               const std::string &min_,
                               const std::string &max_);
        int64_t zremrangebyrank(const std::string &key,
                                int64_t start,
                                int64_t stop);
        int64_t zremrangebyscore(const std::string &key,
                                 const std::string &min_,
                                 const std::string &max_);
        std::list<std::string> zrevrange(const std::string &key,
                                         int64_t start,
                                         int64_t stop);
        std::list<std::pair<std::string, double>> zrevrange_withscores(const std::string &key,
                                                                       int64_t start,
                                                                       int64_t stop);
        std::list<std::string> zrevrangebyscore(const std::string &key,
                                                const std::string &min_,
                                                const std::string &max_);
        std::list<std::string> zrevrangebyscore(const std::string &key,
                                                const std::string &min_,
                                                const std::string &max_,
                                                int64_t offset,
                                                int64_t count);
        std::list<std::pair<std::string, double>> zrevrangebyscore_withscores(const std::string &key,
                                                                              const std::string &min_,
                                                                              const std::string &max_);
        std::list<std::pair<std::string, double>> zrevrangebyscore_withscores(const std::string &key,
                                                                              const std::string &min_,
                                                                              const std::string &max_,
                                                                              int64_t offset,
                                                                              int64_t count);
        boost::optional<int64_t> zrevrank(const std::string &key, const std::string &member);
        double zscore(const std::string &key, const std::string &member);
        int64_t zunionstore(const std::string &destination,
                            std::list<std::string> &&keys,
                            aggregate_type agg=NO_AGGREGATE);
        int64_t zunionstore(const std::string &destination,
                            std::list<std::string> &&keys,
                            std::list<double> &&weights,
                            aggregate_type agg=NO_AGGREGATE);
        
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
        message_queue queue_;
        std::unique_ptr<fiber> subscribing_fiber_;
    };
}}  // End of namespace fibio::redis

#endif
