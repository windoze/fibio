//
//  common_types.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014年 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_common_types_hpp
#define fiberized_io_http_common_common_types_hpp

#include <cstdint>
#include <strings.h>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio { namespace http { namespace common {
    enum class http_version : uint16_t {
        INVALID=0x0000,
        HTTP_0_9=0x0009,
        HTTP_1_0=0x0100,
        HTTP_1_1=0x0101,
    };
    
    std::ostream &operator<<(std::ostream &os, const http_version &v);
    std::istream &operator>>(std::istream &os, http_version &v);
    
    enum class method : uint16_t {
        INVALID,
        DELETE,
        GET,
        HEAD,
        POST,
        PUT,
        /* pathological */
        CONNECT,
        OPTIONS,
        TRACE,
        /* webdav */
        COPY,
        LOCK,
        MKCOL,
        MOVE,
        PROPFIND,
        PROPPATCH,
        SEARCH,
        UNLOCK,
        /* subversion */
        REPORT,
        MKACTIVITY,
        CHECKOUT,
        MERGE,
        /* upnp */
        MSEARCH,
        NOTIFY,
        SUBSCRIBE,
        UNSUBSCRIBE,
        /* RFC-5789 */
        PATCH,
        PURGE,
    };
    
    std::ostream &operator<<(std::ostream &os, const method &v);
    std::istream &operator>>(std::istream &is, method &v);
    
    enum class status_code : uint16_t {
        INVALID                         =0,
        CONTINUE                        =100,
        SWITCHING_PROTOCOLS             =101,
        OK                              =200,
        CREATED                         =201,
        ACCEPTED                        =202,
        NON_AUTHORITATIVE_INFORMATION   =203,
        NO_CONTENT                      =204,
        RESET_CONTENT                   =205,
        PARTIAL_CONTENT                 =206,
        MULTIPLE_CHOICES                =300,
        MOVED_PERMANENTLY               =301,
        FOUND                           =302,
        SEE_OTHER                       =303,
        NOT_MODIFIED                    =304,
        USE_PROXY                       =305,
        //UNUSED                        =306,
        TEMPORARY_REDIRECT              =307,
        BAD_REQUEST                     =400,
        UNAUTHORIZED                    =401,
        PAYMENT_REQUIRED                =402,
        FORBIDDEN                       =403,
        NOT_FOUND                       =404,
        METHOD_NOT_ALLOWED              =405,
        NOT_ACCEPTABLE                  =406,
        PROXY_AUTHENTICATION_REQUIRED   =407,
        REQUEST_TIMEOUT                 =408,
        CONFLICT                        =409,
        GONE                            =410,
        LENGTH_REQUIRED                 =411,
        PRECONDITION_FAILED             =412,
        REQUEST_ENTITY_TOO_LARGE        =413,
        REQUEST_URI_TOO_LONG            =414,
        UNSUPPORTED_MEDIA_TYPE          =415,
        REQUESTED_RANGE_NOT_SATISFIABLE =416,
        EXPECTATION_FAILED              =417,
        UPGRADE_REQUIRED                =426,
        PRECONDITION_REQUIRED           =428,
        TOO_MANY_REQUESTS               =429,
        REQUEST_HEADER_FIELDS_TOO_LARGE =431,
        INTERNAL_SERVER_ERROR           =500,
        NOT_IMPLEMENTED                 =501,
        BAD_GATEWAY                     =502,
        SERVICE_UNAVAILABLE             =503,
        GATEWAY_TIMEOUT                 =504,
        HTTP_VERSION_NOT_SUPPORTED      =505,
    };
    
    struct request_line {
        void clear();
        bool read(std::istream &is);
        bool write(std::ostream &os) const;

        method method_=method::INVALID;
        std::string url_;
        http_version version_=http_version::INVALID;
    };
    
    //std::ostream &operator<<(std::ostream &os, const request_line &v);
    //std::istream &operator>>(std::istream &is, request_line &v);
    
    struct status_line {
        void clear();
        bool read(std::istream &is);
        bool write(std::ostream &os) const;
        void set_status_code(status_code sc, const std::string &msg="");

        http_version version_=http_version::INVALID;
        status_code status_=status_code::INVALID;
        std::string message_;
    };
    
    //std::ostream &operator<<(std::ostream &os, const status_line &v);
    //std::istream &operator>>(std::istream &is, status_line &v);

    typedef std::string header_key_type;
    typedef std::string header_value_type;
    
    struct iless {
        inline bool operator()(const header_key_type &lhs, const header_key_type &rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str())<0;
        }
    };
    
    struct iequal {
        inline bool operator()(const header_key_type &lhs, const header_key_type &rhs) const {
            return strcasecmp(lhs.c_str(), rhs.c_str())==0;
        }
    };

    template<typename Key, typename Value, typename EqualCmp=std::equal_to<Key>>
    struct basic_header_map {
        typedef Key key_type;
        typedef Value mapped_type;
        typedef std::pair<Key, Value> value_type;
        typedef std::vector<value_type> map_type;
        typedef typename map_type::iterator iterator;
        typedef typename map_type::const_iterator const_iterator;
        typedef typename map_type::size_type size_type;
        
	    struct key_equal {
	    	key_equal(const key_type &k)
	    	: key_(k)
	    	{}
            
	        inline bool operator()(const value_type &v) const {
	            return cmp_(key_, v.first);
	        }
	        const key_type &key_;
	        EqualCmp cmp_;
	    };
	    
        struct const_value_proxy {
            explicit const_value_proxy(const basic_header_map *pthis, const key_type &key)
            : pthis_(pthis)
            , key_(key)
            {}
            
            operator mapped_type() const {
                return pthis_->get(key_);
            }
            
            const basic_header_map *pthis_;
            key_type key_;
        };
        
        struct value_proxy {
            explicit value_proxy(basic_header_map *pthis, const key_type &key)
            : pthis_(pthis)
            , key_(key)
            {}
            
            operator mapped_type() const {
                return pthis_->get(key_);
            }
            
            operator const_value_proxy() const {
                return const_value_proxy(pthis_, key_);
            }
            
            void operator=(const mapped_type &v) {
                pthis_->set(key_, v);
            }
            
            void operator=(mapped_type &&v) {
                pthis_->set(key_, v);
            }
            
            void operator+=(const mapped_type &v) {
                pthis_->value_append(key_, v);
            }
            
            void operator+=(mapped_type &&v) {
                pthis_->value_append(key_, v);
            }
            
            bool operator==(const std::string &v) const {
                return pthis_->get(key_)==v;
            }
            
            bool operator!=(const std::string &v) const {
                return pthis_->get(key_)!=v;
            }
            
            basic_header_map *pthis_;
            key_type key_;
        };
        
        value_proxy operator[](const key_type &key)
        { return value_proxy(this, key); }
        
        const_value_proxy operator[](const key_type &key) const
        { return const_value_proxy(this, key); }
        
        iterator begin()
        { return map_.begin(); }
        
        const_iterator begin() const
        { return map_.begin(); }
        
        const_iterator cbegin() const
        { return map_.cbegin(); }
        
        iterator end()
        { return map_.end(); }
        
        const_iterator end() const
        { return map_.end(); }
        
        const_iterator cend() const
        { return map_.cend(); }
        
        iterator find(const key_type &key)
        { return std::find_if(map_.begin(), map_.end(), key_equal(key)); }
        
        const_iterator find(const key_type &key) const
        { return std::find_if(map_.begin(), map_.end(), key_equal(key)); }
        
        bool empty() const
        { return map_.empty; }
        
        void clear()
        { return map_.clear(); }
        
        void erase(iterator i)
        { map_.erase(i); }
        
        void erase(const_iterator i)
        { map_.erase(i); }
        
        void swap(basic_header_map &other)
        { map_.swap(other.map_); }
        
        template<typename K, typename V=mapped_type>
        V get(const K &k, const V &default_value=V()) const {
            const_iterator i=std::find_if(map_.cbegin(), map_.cend(), key_equal(k));
            if(i==map_.end()) {
                return V();
            }
            return i->second;
        }
        
        void set(const key_type &k, const mapped_type &v) {
            iterator i=std::find_if(map_.begin(), map_.end(), key_equal(k));
            if(i==map_.end()) {
                if (!v.empty()) {
                    map_.push_back({k, v});
                }
            } else {
                if (v.empty()) {
                    map_.erase(i);
                } else {
                    i->second=v;
                }
            }
        }
        
        void set(const key_type &k, mapped_type &&v) {
            iterator i=std::find_if(map_.begin(), map_.end(), key_equal(k));
            if(i==map_.end()) {
                if (!v.empty()) {
                    map_.push_back({k, std::move(v)});
                }
            } else {
                if (v.empty()) {
                    map_.erase(i);
                } else {
                    i->second=std::move(v);
                }
            }
        }
        
        void value_append(const key_type &k, const mapped_type &v) {
            iterator i=std::find_if(map_.begin(), map_.end(), key_equal(k));
            if(i==map_.end()) {
                if (!v.empty()) {
                    map_.push_back({k, v});
                }
            } else {
                i->second+=std::move(v);
            }
        }
        
        void value_append(const key_type &k, mapped_type &&v) {
            iterator i=std::find_if(map_.begin(), map_.end(), key_equal(k));
            if(i==map_.end()) {
                if (!v.empty()) {
                    map_.push_back({k, std::move(v)});
                }
            } else {
                i->second+=std::move(v);
            }
        }
        
        template<typename K>
        size_type erase(const K &k) {
            iterator i=std::find_if(map_.begin(), map_.end(), key_equal(k));
            if(i==map_.end()) {
                return 0;
            }
            erase(i);
            return 1;
        }
        
        bool read(std::istream &is) {
            std::string line;
            boost::interprocess::basic_vectorstream<std::string> value_stream;
            auto space=boost::algorithm::is_space();
            header_key_type last_key;
            while (true) {
                // TODO: Add max line limit
                std::getline(is, line);
                boost::algorithm::trim_right(line);
                if (!line.empty()) {
                    if (space(line[0])) {
                        // Leading space, it is a continuation of last line
                        if (last_key.empty()) {
                            // TODO: Error header format
                            return false;
                        } else {
                            // Compress leading spaces (RFC2616,sec4.2)
                            std::string::iterator i=std::find_if_not(line.begin()+1, line.end(), space);
                            //v[last_key].append(i, line.end());
                            (*this)[last_key]+=" ";
                            (*this)[last_key]+=header_value_type(i, line.end());
                        }
                    } else {
                        // key:value
                        std::string::iterator i=std::find(line.begin(), line.end(), ':');
                        header_key_type key(line.begin(), i);
                        header_value_type value(i+1, line.end());
                        boost::algorithm::trim(key);
                        boost::algorithm::trim(value);
                        if (!key.empty() && !value.empty()) {
                            (*this)[key]+=value;
                            last_key=key;
                        }
                    }
                } else {
                    break;
                }
            }
            return true;
        }
        
        bool write(std::ostream &os) const {
            for (auto &p: map_) {
                os << p.first << ": " << p.second << "\r\n";
            }
            return true;
        }

        map_type map_;
    };
    
    typedef basic_header_map<header_key_type, header_value_type, iequal> header_map;
    
    //std::ostream &operator<<(std::ostream &os, const header_map &v);
    //std::istream &operator>>(std::istream &is, header_map &v);
}}} // End of namespace fibio::http::common

namespace fibio { namespace http {
    using common::http_version;
    using common::method;
    using common::status_code;
}}  // End of namespace fibio::http


#endif
