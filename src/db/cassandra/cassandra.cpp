//
// cassandra.cpp
// fibio
//
// Created by Chen Xu on 15/12/11.
// Copyright (c) 2015 0d0a.com. All rights reserved. 
//

#include <stdexcept>
#include <memory>
#include <boost/uuid/uuid.hpp>
#include <fibio/fibers/detail/fiber_base.hpp>
#include <fibio/db/cassandra.hpp>

namespace fibio { namespace db { namespace cassandra {
    namespace detail {
        struct error_impl : public ::fibio::db::cassandra::error
        {
            error_impl(CassError rc)
                    : rc_(rc) { }

            int code() const noexcept override
            {
                return rc_;
            }

            const char* what() const noexcept override
            {
                return cass_error_desc(rc_);
            }

            CassError rc_;
        };

        CassError check(CassError rc)
        {
            if (rc != CASS_OK) {
                throw error_impl(rc);
            }
            return rc;
        }

        struct future_impl
        {
            // Once the future is fully constructed, it's ready
            future_impl(CassFuture* future)
                    : future_(future)
            {
                // Setup callback
                check(cass_future_set_callback(future_,
                                               &future_impl::wait_future,
                                               new future_callback{fibio::fibers::detail::get_current_fiber_ptr()}));

                // Should I?
                cass_future_wait(future_);
                fibio::fibers::detail::get_current_fiber_ptr()->pause();

                check(cass_future_error_code(future_));
            }

            future_impl(CassFuture* future, bool) noexcept
                    : future_(future)
            {
                // Setup callback
                cass_future_set_callback(future_,
                                         &future_impl::wait_future,
                                         new future_callback{fibio::fibers::detail::get_current_fiber_ptr()});

                // Should I?
                cass_future_wait(future_);
                fibio::fibers::detail::get_current_fiber_ptr()->pause();
            }

            ~future_impl()
            {
                cass_future_free(future_);
            }

            operator CassFuture*() const { return future_; }

            struct future_callback
            {
                fibio::fibers::detail::fiber_base::ptr_t current_fiber;
            };

            static void wait_future(CassFuture* future, void* data)
            {
                std::unique_ptr<future_callback> cb(reinterpret_cast<future_callback*>(data));
                cb->current_fiber->resume();
            }

            CassFuture* future_ = nullptr;
        };

        CassRetryPolicy* to_retry_policy(RETRY_POLICY p)
        {
            CassRetryPolicy* ret = nullptr;
            switch (p & 0x0F) {
            case RETRY_POLICY::DEFAULT:
                ret = cass_retry_policy_default_new();
                break;
            case RETRY_POLICY::DOWNGRADING_CONSISTENCY:
                ret = cass_retry_policy_downgrading_consistency_new();
                break;
            case RETRY_POLICY::FALLTHROUGH:
                ret = cass_retry_policy_fallthrough_new();
                break;
            default:
                break;
            }
            if (!ret) throw detail::error_impl(CASS_ERROR_LIB_BAD_PARAMS);
            if (p & RETRY_POLICY::LOGGING) {
                return cass_retry_policy_logging_new(ret);
            }
            return ret;
        }

        // Byte order hack
        union uuid_convertor
        {
            struct
            {
                uint32_t part1;
                uint16_t part2;
                uint16_t part3;
                uint64_t part4;
            };
            CassUuid cu;
            boost::uuids::uuid bu;
        };

        CassUuid conv_uuid(const boost::uuids::uuid& uuid)
        {
            uuid_convertor v;
            v.bu = uuid;
            v.part1 = htonl(v.part1);
            v.part2 = htons(v.part2);
            v.part3 = htons(v.part3);
            v.part4 = htonll(v.part4);
            return v.cu;
        }

        boost::uuids::uuid conv_uuid(const CassUuid& uuid)
        {
            uuid_convertor v;
            v.cu = uuid;
            v.part1 = ntohl(v.part1);
            v.part2 = ntohs(v.part2);
            v.part3 = ntohs(v.part3);
            v.part4 = ntohll(v.part4);
            return v.bu;
        }
    }

    using detail::check;
    using detail::to_retry_policy;

    ////////////////////////////////////////////////////////////////////////////////////
    // cluster
    ////////////////////////////////////////////////////////////////////////////////////
    cluster::cluster()
            : base_type(cass_cluster_new())
    {
    }

    cluster::cluster(const std::string& contact_points)
            : base_type(cass_cluster_new())
    {
        set_contact_points(contact_points);
    }

    void cluster::set_port(uint16_t port)
    {
        check(cass_cluster_set_port(*this, port));
    }

    void cluster::set_protocol_version(int protocol_version)
    {
        check(cass_cluster_set_protocol_version(*this, protocol_version));
    }

    void cluster::set_connect_timeout(std::chrono::steady_clock::duration timeout)
    {
        cass_cluster_set_connect_timeout(*this, std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());

    }

    void cluster::set_request_timeout(std::chrono::steady_clock::duration timeout)
    {
        cass_cluster_set_request_timeout(*this, std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());

    }

    void cluster::set_contact_points(const std::string& contact_points)
    {
        cass_cluster_set_contact_points_n(impl_.get(), contact_points.c_str(), contact_points.size());
    }

    void cluster::set_credentials(const std::string& username, const std::string& password)
    {
        cass_cluster_set_credentials_n(*this, username.c_str(), username.size(), password.c_str(), password.size());
    }

    void cluster::set_load_balance_round_robin()
    {
        cass_cluster_set_load_balance_round_robin(*this);
    }

    void cluster::set_load_balance_dc_aware(const std::string& local_dc,
                                            unsigned used_hosts_per_remote_dc,
                                            bool allow_remote_dcs_for_local_cl)
    {
        cass_cluster_set_load_balance_dc_aware_n(*this,
                                                 local_dc.c_str(),
                                                 local_dc.size(),
                                                 used_hosts_per_remote_dc,
                                                 allow_remote_dcs_for_local_cl
                                                 ? cass_bool_t::cass_true
                                                 : cass_bool_t::cass_false);
    }

    void cluster::set_token_aware_routing(bool enabled)
    {
        cass_cluster_set_token_aware_routing(*this, enabled ? cass_bool_t::cass_true
                                                            : cass_bool_t::cass_false);
    }

    void cluster::set_latency_aware_routing(bool enabled)
    {
        cass_cluster_set_latency_aware_routing(*this, enabled ? cass_bool_t::cass_true
                                                              : cass_bool_t::cass_false);
    }

    void cluster::set_latency_aware_routing_settings(cass_double_t exclusion_threshold,
                                                     std::chrono::steady_clock::duration scale_ms,
                                                     std::chrono::steady_clock::duration retry_period_ms,
                                                     std::chrono::steady_clock::duration update_rate_ms,
                                                     cass_uint64_t min_measured)
    {
        cass_cluster_set_latency_aware_routing_settings(
                *this,
                exclusion_threshold,
                std::chrono::duration_cast<std::chrono::milliseconds>(scale_ms).count(),
                std::chrono::duration_cast<std::chrono::milliseconds>(retry_period_ms).count(),
                std::chrono::duration_cast<std::chrono::milliseconds>(update_rate_ms).count(),
                min_measured);
    }

    void cluster::set_whitelist_filtering(const std::string& hosts)
    {
        cass_cluster_set_whitelist_filtering_n(*this, hosts.c_str(), hosts.size());
    }

    void cluster::set_tcp_nodelay(bool enabled)
    {
        cass_cluster_set_tcp_nodelay(*this, enabled ? cass_bool_t::cass_true
                                                    : cass_bool_t::cass_false);
    }

    void cluster::set_connection_idle_timeout(std::chrono::steady_clock::duration timeout)
    {
        cass_cluster_set_connection_idle_timeout(*this,
                                                 std::chrono::duration_cast<std::chrono::seconds>(timeout).count());
    }

    void cluster::set_tcp_keepalive(bool enabled, std::chrono::steady_clock::duration delay)
    {
        cass_cluster_set_tcp_keepalive(*this,
                                       enabled ? cass_bool_t::cass_true
                                               : cass_bool_t::cass_false,
                                       std::chrono::duration_cast<std::chrono::seconds>(delay).count());
    }

    void cluster::set_connection_heartbeat_interval(std::chrono::steady_clock::duration timeout)
    {
        cass_cluster_set_connection_heartbeat_interval(
                *this,
                std::chrono::duration_cast<std::chrono::seconds>(timeout).count());
    }

    session cluster::connect()
    {
        ::CassSession* s = ::cass_session_new();
        detail::future_impl(::cass_session_connect(s, impl_.get()));
        return session(s);
    }

    session cluster::connect(const std::string& keyspace)
    {
        ::CassSession* s = ::cass_session_new();
        detail::future_impl(::cass_session_connect_keyspace_n(s,
                                                              impl_.get(),
                                                              keyspace.c_str(),
                                                              keyspace.size()));
        return session(s);
    }

    void cluster::set_retry_policy(RETRY_POLICY p)
    {
        retry_.impl_.reset(to_retry_policy(p));
        cass_cluster_set_retry_policy(*this, retry_);
    }

    void cluster::set_use_schema(bool enabled)
    {
        cass_cluster_set_use_schema(*this, enabled ? cass_bool_t::cass_true
                                                   : cass_bool_t::cass_false);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // session
    ////////////////////////////////////////////////////////////////////////////////////

    session::session(::CassSession* session)
            : base_type(session)
    {
    }

    void session::close()
    {
        cass_session_close(*this);
    }

    result_set session::execute(const statement& s)
    {
        return result_set(cass_future_get_result(detail::future_impl(cass_session_execute(*this, s))));
    }

    result_set session::execute(const batch& b)
    {
        return result_set(cass_future_get_result(detail::future_impl(cass_session_execute_batch(*this, b))));
    }

    prepared session::prepare(const std::string& query)
    {
        return prepared(cass_future_get_prepared(detail::future_impl(
                cass_session_prepare_n(*this, query.c_str(), query.size())
        )));
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // prepared
    ////////////////////////////////////////////////////////////////////////////////////

    prepared::prepared(const ::CassPrepared* p)
            : base_type(p)
    {
    }

    statement prepared::bind()
    {
        return statement(cass_prepared_bind(*this));
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // statement
    ////////////////////////////////////////////////////////////////////////////////////
    statement::statement(::CassStatement* s)
            : base_type(s)
            , retry_(nullptr)
    {
    }

    statement::statement(const std::string& query, size_t parameter_count)
            : base_type(cass_statement_new_n(query.c_str(),
                                             query.size(),
                                             parameter_count))
            , retry_(nullptr)
    {
    }

    void statement::set_consistency(CassConsistency c)
    {
        check(cass_statement_set_consistency(*this, c));
    }

    void statement::set_serial_consistency(CassConsistency c)
    {
        check(cass_statement_set_consistency(*this, c));
    }

    void statement::set_paging_size(int s)
    {
        check(cass_statement_set_paging_size(*this, s));
    }

    void statement::set_retry_policy(RETRY_POLICY p)
    {
        retry_.impl_.reset(to_retry_policy(p));
        check(cass_statement_set_retry_policy(*this, retry_));
    }

    void statement::bind(size_t index)
    {
        check(cass_statement_bind_null(*this, index));
    }

    void statement::bind(size_t index, int8_t v)
    {
        check(cass_statement_bind_int8(*this, index, v));
    }

    void statement::bind(size_t index, int16_t v)
    {
        check(cass_statement_bind_int16(*this, index, v));
    }

    void statement::bind(size_t index, int32_t v)
    {
        check(cass_statement_bind_int32(*this, index, v));
    }

    void statement::bind(size_t index, uint32_t v)
    {
        check(cass_statement_bind_uint32(*this, index, v));
    }

    void statement::bind(size_t index, int64_t v)
    {
        check(cass_statement_bind_int64(*this, index, v));
    }

    void statement::bind(size_t index, float v)
    {
        check(cass_statement_bind_float(*this, index, v));
    }

    void statement::bind(size_t index, double v)
    {
        check(cass_statement_bind_double(*this, index, v));
    }

    void statement::bind(size_t index, bool v)
    {
        check(cass_statement_bind_bool(*this, index, v ? cass_bool_t::cass_true
                                                       : cass_bool_t::cass_false));
    }

    void statement::bind(size_t index, const std::string& v)
    {
        check(cass_statement_bind_string_n(*this, index, v.c_str(), v.size()));
    }

    void statement::bind(size_t index, const boost::uuids::uuid& v)
    {
        check(cass_statement_bind_uuid(*this, index, detail::conv_uuid(v)));
    }
    ////////////////////////////////////////////////////////////////////////////////////
    // batch
    ////////////////////////////////////////////////////////////////////////////////////

    batch::batch(::CassBatchType batch_type)
            : base_type(cass_batch_new(batch_type))
    {
    }

    void batch::set_consistency(CassConsistency c)
    {
        check(cass_batch_set_consistency(*this, c));
    }

    void batch::set_serial_consistency(CassConsistency c)
    {
        check(cass_batch_set_consistency(*this, c));
    }

    void batch::set_retry_policy(RETRY_POLICY p)
    {
        retry_.impl_.reset(to_retry_policy(p));
        check(cass_batch_set_retry_policy(*this, retry_));
    }

    void batch::add_statement(const statement& s)
    {
        cass_batch_add_statement(*this, s);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // result_set
    ////////////////////////////////////////////////////////////////////////////////////

    result_set::result_set(const ::CassResult* r)
            : base_type(r)
    {
    }

    result_set::iterator result_set::begin() const
    {
        return iterator(*this);
    }

    result_set::iterator result_set::end() const
    {
        return std::move(iterator());
    }

    result_set::const_iterator result_set::cbegin() const
    {
        return begin();
    }

    result_set::const_iterator result_set::cend() const
    {
        return end();
    }

    size_t result_set::size() const
    {
        return cass_result_row_count(*this);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // result_set_iterator
    ////////////////////////////////////////////////////////////////////////////////////
    result_set_iterator::result_set_iterator()
            : base_type()
            , current_row_(nullptr)
    {
    }

    result_set_iterator::result_set_iterator(const ::CassResult* r)
            : base_type(cass_iterator_from_result(r))
            , current_row_(impl_.get())
    {
        current_row_.result_ = r;
        cass_iterator_next(*this);
    }

    const row& result_set_iterator::operator*() const
    {
        return current_row_;
    }

    const row* result_set_iterator::operator->() const
    {
        return &current_row_;
    }

    result_set_iterator& result_set_iterator::operator++()
    {
        if (!cass_iterator_next(*this)) {
            current_row_.iterator_ = nullptr;
            impl_.reset();
        }
        return *this;
    }

    bool result_set_iterator::operator==(const result_set_iterator& other) const
    {
        return (!impl_ || !current_row_.iterator_) && (!other.impl_ || !other.current_row_.iterator_);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // row
    ////////////////////////////////////////////////////////////////////////////////////
    row::row(const CassIterator* i)
            : iterator_(i)
    {
    }

    value row::operator[](size_t index) const
    {
        return value(cass_row_get_column(cass_iterator_get_row(iterator_), index));
    }

    value row::operator[](const std::string& name) const
    {
        return value(cass_row_get_column_by_name_n(cass_iterator_get_row(iterator_), name.c_str(), name.size()));
    }

    size_t row::size() const
    {
        return cass_result_row_count(result_);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // value
    ////////////////////////////////////////////////////////////////////////////////////
    value::value(const CassValue* v)
            : impl_(v)
    {
    }

    CassValueType value::get_type() const
    {
        return CassValueType(cass_value_type(impl_));
    }

    int8_t value::get_int8() const
    {
        int8_t v;
        check(cass_value_get_int8(impl_, &v));
        return v;
    }

    int16_t value::get_int16() const
    {
        int16_t v;
        check(cass_value_get_int16(impl_, &v));
        return v;
    }

    int32_t value::get_int32() const
    {
        int32_t v;
        check(cass_value_get_int32(impl_, &v));
        return v;
    }

    uint32_t value::get_uint32() const
    {
        uint32_t v;
        check(cass_value_get_uint32(impl_, &v));
        return v;
    }

    int64_t value::get_int64() const
    {
        int64_t v;
        check(cass_value_get_int64(impl_, &v));
        return v;
    }

    float value::get_float() const
    {
        float v;
        check(cass_value_get_float(impl_, &v));
        return v;
    }

    double value::get_double() const
    {
        double v;
        check(cass_value_get_double(impl_, &v));
        return v;
    }

    boost::uuids::uuid value::get_uuid() const
    {
        // Byte order hack
        CassUuid cu;
        check(cass_value_get_uuid(impl_, &cu));
        return detail::conv_uuid(cu);
    }

    std::string value::get_string() const
    {
        const char* str;
        size_t len;
        check(cass_value_get_string(impl_, &str, &len));
        return std::string(str, len);
    }
}}} // End of namespace fibio::db::cassandra
