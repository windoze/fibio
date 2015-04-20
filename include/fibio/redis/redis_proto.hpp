//
//  redis_proto.h
//  redis_proto
//
//  Created by Chen Xu on 14/9/25.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio_redis_redis_proto_hpp__
#define __fibio_redis_redis_proto_hpp__

#include <list>
#include <istream>
#include <ostream>
#include <exception>
#include <chrono>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

namespace fibio { namespace redis {
    enum redis_data_type {
        RDT_SIMPLE_STRING,
        RDT_ERROR,
        RDT_INTEGER,
        RDT_BULK_STRING,
        RDT_NIL,
        RDT_ARRAY
    };
    
    class redis_error : public std::runtime_error{
        using std::runtime_error::runtime_error;
    };

    struct simple_string : public std::string {
        typedef std::string base_type;
        using base_type::base_type;
        simple_string()=default;
        simple_string(const base_type &s)
        : simple_string(static_cast<const simple_string &>(s))
        {}
        simple_string(base_type &&s)
        : simple_string(static_cast<simple_string &&>(s))
        {}
    };
    
    struct error : public std::string {
        typedef std::string base_type;
        using base_type::base_type;
        error()=default;
        error(const base_type &s)
        : error(static_cast<const error &>(s))
        {}
        error(base_type &&s)
        : error(static_cast<error &&>(s))
        {}
    };
    
    struct bulk_string : public std::string {
        typedef std::string base_type;
        using base_type::base_type;
        bulk_string()=default;
        bulk_string(const base_type &s)
        : bulk_string(static_cast<const bulk_string &>(s))
        {}
        bulk_string(base_type &&s)
        : bulk_string(static_cast<bulk_string &&>(s))
        {}
    };
    
    struct array;
    
    typedef boost::variant<std::nullptr_t, int64_t, simple_string, error, bulk_string, boost::recursive_wrapper<array> > redis_data;
    
    const redis_data nil=redis_data(nullptr);
    
    struct array : std::list<redis_data> {
        typedef std::list<redis_data> base_type;
        using base_type::base_type;
    };
    
    typedef boost::optional<std::string> nullable_result;
    
    struct nullable_result_getter : public boost::static_visitor<nullable_result> {
        nullable_result operator()(const bulk_string &d) const {
            return nullable_result(boost::get<bulk_string>(d));
        }
        template<typename T>
        nullable_result operator()(const T &d) const {
            return nullable_result();
        }
    };
    
    namespace detail {
        inline void make_array(array &a) {}
        
        inline void make_array_impl(array &a, bulk_string &&s) {
            a.push_back(std::move(s));
        }
        inline void make_array_impl(array &a, int64_t n) {
            a.push_back(n);
        }
        template<std::size_t N>
        inline void make_array_impl(array &a, const char(&s)[N]) {
            return make_array_impl(a, bulk_string(&(s[0])));
        }
        inline void make_array_impl(array &a, double d) {
            a.push_back(boost::lexical_cast<bulk_string>(d));
        }
        inline void make_array_impl(array &a, std::string &&s) {
            a.push_back(bulk_string(std::move(s)));
        }
        inline void make_array_impl(array &a, std::chrono::system_clock::time_point t) {
            a.push_back(int64_t(std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count()));
        }
        inline void make_array_impl(array &a, std::chrono::system_clock::duration t) {
            a.push_back(int64_t(std::chrono::duration_cast<std::chrono::microseconds>(t).count()));
        }
        template<typename T>
        void make_array_impl(array &a, boost::optional<T> &&v) {
            if(v) { make_array_impl(a, std::move(*v)); }
        }
        inline void make_array_impl(array &a, std::list<std::string> &&l) {
            for(auto &i : l) {
                a.push_back(bulk_string(std::move(i)));
            }
        }
        inline void make_array_impl(array &a, std::list<std::pair<std::string,std::string>> &&l) {
            for(auto &i : l) {
                a.push_back(bulk_string(std::move(i.first)));
                a.push_back(bulk_string(std::move(i.second)));
            }
        }
        inline void make_array_impl(array &a, std::list<std::pair<double,std::string>> &&l) {
            for(auto &i : l) {
                a.push_back(boost::lexical_cast<bulk_string>(i.first));
                a.push_back(bulk_string(std::move(i.second)));
            }
        }
        template<typename U, typename V>
        void make_array_impl(array &a, std::pair<U, V> p) {
            a.push_back(boost::lexical_cast<bulk_string>(p.first));
            a.push_back(boost::lexical_cast<bulk_string>(p.second));
        }
        
        template<typename T, typename... Args>
        void make_array(array &a, T &&t, Args&&... args) {
            make_array_impl(a, std::move(t));
            make_array(a, std::forward<Args>(args)...);
        }
    }   // End of namespace detail
    
    template<typename... Args>
    array make_array(Args&&... args) {
        array a;
        detail::make_array(a, std::forward<Args>(args)...);
        return a;
    }
    
    template<typename T>
    array &operator<<(array &a, T &&t) {
        detail::make_array_impl(a, std::forward<T>(t));
        return a;
    }

    template<typename T>
    T extract(redis_data &&d) {
        return std::move(boost::get<T>(d));
    }
    
    template<>
    inline redis_data extract<redis_data>(redis_data &&d) {
        return std::move(d);
    }
    
    template<>
    inline bool extract<bool>(redis_data &&d) {
        struct to_bool : boost::static_visitor<bool> {
            bool operator()(bulk_string &s) const {
                return !s.empty();
            }
            bool operator()(simple_string &s) const {
                return !s.empty();
            }
            bool operator()(array &s) const {
                return !s.empty();
            }
            bool operator()(error &s) const {
                return false;
            }
            bool operator()(int64_t s) const {
                return s!=0;
            }
            bool operator()(std::nullptr_t) const {
                return false;
            }
        };
        return boost::apply_visitor(to_bool(), d);
    }
    
    template<>
    inline double extract<double>(redis_data &&d) {
        return boost::lexical_cast<double>(extract<bulk_string>(std::forward<redis_data>(d)));
    }
    
    template<>
    inline nullable_result extract<nullable_result>(redis_data &&d) {
        return boost::apply_visitor(nullable_result_getter(), d);
    }
    
    template<>
    inline std::list<nullable_result> extract<std::list<nullable_result>>(redis_data &&d) {
        std::list<nullable_result> r;
        for (auto &i : boost::get<array>(d)) {
            r.push_back(extract<nullable_result>(std::move(i)));
        }
        return r;
    }
    
    template<>
    inline std::chrono::system_clock::time_point extract<std::chrono::system_clock::time_point>(redis_data &&d) {
        return std::chrono::system_clock::from_time_t(time_t(boost::get<int64_t>(d)));
    }
    
    template<>
    inline std::list<std::string> extract<std::list<std::string>>(redis_data &&d) {
        std::list<std::string> r;
        for (auto &i : boost::get<array>(d)) {
            r.push_back(extract<bulk_string>(std::move(i)));
        }
        return r;
    }
    
    template<>
    inline std::list<std::pair<std::string, int64_t>> extract<std::list<std::pair<std::string, int64_t>>>(redis_data &&d) {
        array &a=boost::get<array>(d);
        std::list<std::pair<std::string, int64_t>> ret;
        auto i=a.begin();
        while (i!=a.end()) {
            std::pair<std::string, int64_t> p;
            p.first.assign(std::move(boost::get<bulk_string>(*i)));
            ++i;
            p.second=extract<int64_t>(std::move(*i));
            ++i;
            ret.push_back(std::move(p));
        }
        return ret;
    }

    template<>
    inline std::list<std::pair<std::string, double>> extract<std::list<std::pair<std::string, double>>>(redis_data &&d) {
        array &a=boost::get<array>(d);
        std::list<std::pair<std::string, double>> ret;
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
    
    template<>
    inline void extract<void>(redis_data &&d)
    {}
    
    const redis_data &check_result(const redis_data &d);
    redis_data check_result(redis_data &d);
    redis_data_type data_type(const redis_data &d);
    std::ostream &operator<<(std::ostream &os, const redis_data &v);
    std::istream &operator>>(std::istream &is, redis_data &v);
}}  // End of namespace fibio::redis

#endif /* defined(__fibio_redis_redis_proto_hpp__) */
