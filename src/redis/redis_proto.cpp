//
//  redis_proto.cpp
//  redis_proto
//
//  Created by Chen Xu on 14/9/25.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/redis/redis_proto.hpp>

namespace fibio { namespace redis {
    namespace detail {
        constexpr char C_SIMPLE_STRING = '+';
        constexpr char C_ERROR = '-';
        constexpr char C_INTEGER = ':';
        constexpr char C_BULK_STRING = '$';
        constexpr char C_NULL_STRING = '$';
        constexpr char C_ARRAY = '*';
        
        struct data_type_vistor : boost::static_visitor<redis_data_type> {
            redis_data_type operator()(const std::nullptr_t &) const
            { return RDT_NIL; }
            redis_data_type operator()(const int64_t &) const
            { return RDT_INTEGER; }
            redis_data_type operator()(const simple_string &) const
            { return RDT_SIMPLE_STRING; }
            redis_data_type operator()(const error &) const
            { return RDT_ERROR; }
            redis_data_type operator()(const bulk_string &) const
            { return RDT_BULK_STRING; }
            redis_data_type operator()(const array &) const
            { return RDT_ARRAY; }
        };
        
        struct result_checker : boost::static_visitor<> {
            void operator()(const error &err) const {
                BOOST_THROW_EXCEPTION(redis_error(err));
            }
            template<typename T>
            void operator()(const T&) const
            {}
        };
        
        struct output_vistor : boost::static_visitor<> {
            output_vistor(std::ostream &os)
            : os_(os)
            {}
            
            void operator()(const std::nullptr_t &) const {
                os_ << C_NULL_STRING << "-1\r\n";
            }
            
            void operator()(const int64_t &v) const {
                os_ << C_INTEGER << v << "\r\n";
            }
            
            void operator()(const simple_string &v) const {
                os_ << C_SIMPLE_STRING;
                os_.write(&v[0], v.size());
                os_ << "\r\n";
            }
            
            void operator()(const error &v) const {
                os_ << C_ERROR;
                os_.write(&v[0], v.size());
                os_ << "\r\n";
            }
            
            void operator()(const bulk_string &v) const {
                os_ << C_BULK_STRING << v.size() << "\r\n";
                os_.write(&v[0], v.size());
                os_ << "\r\n";
            }
            
            void operator()(const array &v) const {
                os_ << C_ARRAY << v.size() << "\r\n";
                output_vistor sub_visitor(os_);
                for (const redis_data &e : v) {
                    boost::apply_visitor(sub_visitor, e);
                }
            }
            
            std::ostream &os_;
        };
        
        
        bool parse_redis_data(std::istream &is, redis_data &v) {
            char c;
            is.read(&c, 1);
            if (is.eof() || is.bad()) {
                return false;
            }
            
            char buf[2];
            switch (c) {
                case C_SIMPLE_STRING:
                {
                    redis_data d(simple_string(256, ' '));
                    std::getline(is, boost::get<simple_string>(d), '\r');
                    v.swap(d);
                    // Consume last '\n', getline() already consumed '\r'
                    is.read(buf, 1);
                    break;
                }
                case C_ERROR:
                {
                    redis_data d(error(256, ' '));
                    std::getline(is, boost::get<error>(d), '\r');
                    v.swap(d);
                    // Consume last '\n', getline() already consumed '\r'
                    is.read(buf, 1);
                    break;
                }
                    
                case C_INTEGER:
                {
                    int64_t i;
                    is >> i;
                    v=i;
                    // Consume last "\r\n"
                    is.read(buf, 2);
                    break;
                }
                    
                case C_BULK_STRING:
                {
                    int64_t i;
                    is >> i;
                    // Consume "\r\n"
                    is.read(buf, 2);
                    if (i==-1) {
                        // Null Bulk String
                        v=nil;
                    } else {
                        // Normal Bulk String
                        redis_data d(bulk_string(i, ' '));
                        is.read(&(boost::get<bulk_string>(d))[0], i);
                        v.swap(d);
                        // Consume last "\r\n"
                        is.read(buf, 2);
                    }
                    break;
                }
                    
                case C_ARRAY:
                {
                    int64_t i;
                    is >> i;
                    // Consume "\r\n"
                    is.read(buf, 2);
                    redis_data d(array({}));
                    for (int64_t n=0; n<i; n++) {
                        redis_data e;
                        if(!parse_redis_data(is, e))
                            return false;
                        boost::get<array>(d).push_back(std::move(e));
                    }
                    v.swap(d);
                    break;
                }
                    
                default:
                    // TODO: Inline command
                    assert(false);
                    return false;
            }
            return true;
        }
    }   // End of namespace redis_proto::detail
    
    redis_data_type data_type(const redis_data &d) {
        return boost::apply_visitor(detail::data_type_vistor(), d);
    }
    
    std::ostream &operator<<(std::ostream &os, const redis_data &v) {
        boost::apply_visitor(detail::output_vistor(os), v);
        return os;
    }
    
    std::istream &operator>>(std::istream &is, redis_data &v) {
        detail::parse_redis_data(is, v);
        return is;
    }

    const redis_data &check_result(const redis_data &d) {
        boost::apply_visitor(detail::result_checker(), d);
        return d;
    }
    
    redis_data check_result(redis_data &d) {
        boost::apply_visitor(detail::result_checker(), d);
        return std::move(d);
    }
}}  // End of namespace fibio::redis
