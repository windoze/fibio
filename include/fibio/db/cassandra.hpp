//
//  cassandra.hpp
//  fibio
//
//  Created by Chen Xu on 15/12/10.
//  Copyright (c) 2015 0d0a.com. All rights reserved.
//

#ifndef fibio_db_cassandra_hpp
#define fibio_db_cassandra_hpp

#include <stdexcept>
#include <iterator>
#include <chrono>
#include <cassandra.h>
#include <boost/uuid/uuid.hpp>
#include <fibio/fibers/detail/fiber_base.hpp>

namespace fibio { namespace db { namespace cassandra {

    class error : public std::exception
    {
    public:
        virtual int code() const noexcept = 0;
    };

    typedef enum
    {
        DEFAULT = 1,
        DOWNGRADING_CONSISTENCY = 2,
        FALLTHROUGH = 3,
        LOGGING = 0x80
    } RETRY_POLICY;

    namespace detail {
        template<typename T>
        struct deleter;

        template<>
        struct deleter<::CassCluster>
        {
            void operator()(::CassCluster* p) const { ::cass_cluster_free(p); }
        };

        template<>
        struct deleter<::CassRetryPolicy>
        {
            void operator()(::CassRetryPolicy* p) const { ::cass_retry_policy_free(p); }
        };

        template<>
        struct deleter<::CassSession>
        {
            void operator()(::CassSession* p) const { ::cass_session_free(p); }
        };

        template<>
        struct deleter<const ::CassPrepared>
        {
            void operator()(const ::CassPrepared* p) const { ::cass_prepared_free(p); }
        };

        template<>
        struct deleter<::CassStatement>
        {
            void operator()(::CassStatement* p) const { ::cass_statement_free(p); }
        };

        template<>
        struct deleter<::CassBatch>
        {
            void operator()(::CassBatch* p) const { ::cass_batch_free(p); }
        };

        template<>
        struct deleter<const ::CassResult>
        {
            void operator()(const ::CassResult* p) const { ::cass_result_free(p); }
        };

        template<>
        struct deleter<::CassIterator>
        {
            void operator()(::CassIterator* p) const { ::cass_iterator_free(p); }
        };

        template<typename T>
        struct CassBase
        {
            CassBase() = default;

            CassBase(T* p)
                    : impl_(p)
            {
            }

            operator T*() const { return impl_.get(); }

            std::unique_ptr<T, deleter<T>> impl_;
        };
    }

    struct value
    {
        value(const CassValue*);

        ::CassValueType get_type() const;

        int8_t get_int8() const;

        int16_t get_int16() const;

        int32_t get_int32() const;

        uint32_t get_uint32() const;

        int64_t get_int64() const;

        float get_float() const;

        double get_double() const;

        std::string get_string() const;

        boost::uuids::uuid get_uuid() const;

        const CassValue* impl_;
    };

    struct row
    {
        row(const CassIterator*);

        value operator[](size_t index) const;

        value operator[](const std::string& name) const;

        size_t size() const;

        const CassIterator* iterator_ = nullptr;
        const CassResult* result_ = nullptr;
    };

    struct result_set_iterator : detail::CassBase<::CassIterator>, std::iterator<std::input_iterator_tag,
                                                                                 row,
                                                                                 std::ptrdiff_t,
                                                                                 const row*,
                                                                                 const row&>
    {
        result_set_iterator();

        result_set_iterator(const ::CassResult*);

        result_set_iterator& operator++();

        bool operator==(const result_set_iterator& other) const;

        bool operator!=(const result_set_iterator& other) const
        {
            return !operator==(other);
        }

        const row& operator*() const;

        const row* operator->() const;

        row current_row_;

        typedef detail::CassBase<::CassIterator> base_type;
    };

    struct result_set : detail::CassBase<const ::CassResult>
    {
        typedef row value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef result_set_iterator iterator;
        typedef result_set_iterator const_iterator;

        result_set(const ::CassResult*);

        iterator begin() const;

        iterator end() const;

        const_iterator cbegin() const;

        const_iterator cend() const;

        size_t size() const;

        typedef detail::CassBase<const ::CassResult> base_type;
    };

    struct statement : detail::CassBase<::CassStatement>
    {
        statement(::CassStatement*);

        statement(const std::string& query, size_t parameter_count);

        void set_consistency(CassConsistency);

        void set_serial_consistency(CassConsistency);

        void set_paging_size(int);

        void set_retry_policy(RETRY_POLICY);

        void bind(size_t index);    // bind_null;

        void bind(size_t index, int8_t);

        void bind(size_t index, int16_t);

        void bind(size_t index, int32_t);

        void bind(size_t index, uint32_t);

        void bind(size_t index, int64_t);

        void bind(size_t index, float);

        void bind(size_t index, double);

        void bind(size_t index, bool);

        void bind(size_t index, const std::string&);

        void bind(size_t index, const boost::uuids::uuid&);

        typedef detail::CassBase<::CassStatement> base_type;

        detail::CassBase<::CassRetryPolicy> retry_;
    };

    struct prepared : detail::CassBase<const ::CassPrepared>
    {
        prepared(const ::CassPrepared*);

        statement bind();

        typedef detail::CassBase<const ::CassPrepared> base_type;
    };

    struct batch : detail::CassBase<::CassBatch>
    {
        batch(::CassBatchType batch_type);

        void set_consistency(CassConsistency);

        void set_serial_consistency(CassConsistency);

        void set_retry_policy(RETRY_POLICY);

        void add_statement(const statement&);

        typedef detail::CassBase<::CassBatch> base_type;

        detail::CassBase<::CassRetryPolicy> retry_;
    };

    struct session : detail::CassBase<::CassSession>
    {
        session(::CassSession*);

        void close();

        prepared prepare(const std::string& query);

        result_set execute(const statement&);

        result_set execute(const batch&);

        typedef detail::CassBase<::CassSession> base_type;
    };

    struct cluster : detail::CassBase<::CassCluster>
    {
        cluster();

        cluster(const std::string& contact_points);

        void set_contact_points(const std::string&);

        void set_port(uint16_t port);

        void set_protocol_version(int protocol_version);

        void set_connect_timeout(std::chrono::steady_clock::duration timeout);

        void set_request_timeout(std::chrono::steady_clock::duration timeout);

        void set_credentials(const std::string& username, const std::string& password);

        void set_load_balance_round_robin();

        void set_load_balance_dc_aware(const std::string& local_dc,
                                       unsigned used_hosts_per_remote_dc,
                                       bool allow_remote_dcs_for_local_cl);

        void set_token_aware_routing(bool enabled);

        void set_latency_aware_routing(bool enabled);

        void set_latency_aware_routing_settings(cass_double_t exclusion_threshold,
                                                std::chrono::steady_clock::duration scale_ms,
                                                std::chrono::steady_clock::duration retry_period_ms,
                                                std::chrono::steady_clock::duration update_rate_ms,
                                                cass_uint64_t min_measured);

        void set_whitelist_filtering(const std::string& hosts);

        void set_tcp_nodelay(bool enabled);

        void set_tcp_keepalive(bool enabled, std::chrono::steady_clock::duration delay);

        void set_connection_idle_timeout(std::chrono::steady_clock::duration timeout);

        void set_connection_heartbeat_interval(std::chrono::steady_clock::duration timeout);

        session connect();

        session connect(const std::string& keyspace);

        void set_retry_policy(RETRY_POLICY);

        void set_use_schema(bool enabled);

        typedef detail::CassBase<::CassCluster> base_type;

        detail::CassBase<::CassRetryPolicy> retry_;
    };

}}}    // End of namespace fibio::db::cassandra

#endif	// fibio_db_cassandra_hpp
