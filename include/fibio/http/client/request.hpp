//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014年 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_client_request_hpp
#define fiberized_io_http_client_request_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/request.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio { namespace http { namespace client {
    struct request : common::request {
        request()=default;
        
        request(const request &other)
        : common::request(other)
        {}
        
        request &operator=(const request &other) {
            common::request::operator=(other);
            return *this;
        }
        
        void clear();
        
        size_t get_content_length() const;
        
        const std::string &get_body() const;
        
        std::ostream &body_stream();
        
        template<typename T>
        void set_body(const T &t, const std::string &content_type=common::content_type<T>::name) {
            body_stream_ << t;
            if (get_content_type().empty())
                set_content_type(content_type);
        }

        bool write(std::ostream &os) const;

        boost::interprocess::basic_ovectorstream<std::string> body_stream_;
    };
    
    inline std::ostream &operator<<(std::ostream &os, const request &v) {
        v.write(os);
        return os;
    }
}}} // End of namespace fibio::http::client

#endif
