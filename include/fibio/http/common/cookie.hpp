//
//  cookie.hpp
//  fibio
//
//  Created by Chen Xu on 14/10/25.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_cookie_hpp
#define fibio_http_common_cookie_hpp

#include <cstdint>
#include <string>
#include <map>
#include <list>
#include <chrono>
#include <boost/lexical_cast.hpp>
#include <fibio/http/common/common_types.hpp>
#include <fibio/http/common/request.hpp>
#include <fibio/http/common/response.hpp>

namespace fibio { namespace http { namespace common {
    typedef std::chrono::time_point<std::chrono::system_clock> timepoint_type;
    
    struct cookie {
        std::string name;
        std::string value;
        std::string path;
        std::string domain;
        timepoint_type expires;
        bool secure=false;
        bool http_only=false;
        
        cookie(const std::string &s);
        cookie()=default;
        
        template<typename T>
        T get() const {
            return boost::lexical_cast<T>(value);
        }
        
        template<typename T>
        void set(const T &t) {
            value=boost::lexical_cast<std::string>(t);
        }
        
        bool expired() const {
            return expires<=std::chrono::system_clock::now();
        }
        
        bool effective(const std::string &url) const;
        bool effective(const parsed_url_type &url) const;
        
        std::string to_string() const;
        static std::pair<std::string, cookie> from_string(const std::string &s);
    };
    
    typedef std::map<std::string, cookie> cookie_map;
    
    bool is_subdomain(const std::string &sd, const std::string &d);
    
    void parse_cookie(const header_map &h, cookie_map &cookies, bool set);
    
    struct cookie_jar {
        typedef std::map<std::string, common::cookie_map, common::iless> cookie_storage;
        
        // Save cookies within response into cookie jar
        void save_cookie(const std::string &full_url, const response &resp);
        // Load effective cookies into request
        void load_cookie(const std::string &full_url, request &req);
        
    private:
        void add_cookie(const common::cookie &c);
        cookie_storage storage_;
    };
}}} // End of namespace fibio::http::common

#endif
