//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_response_hpp
#define fibio_http_server_response_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/response.hpp>

namespace fibio { namespace http {
    struct server_response : common::response {
        server_response()=default;

        server_response(const server_response &other)
        : common::response(other)
        , raw_stream_(other.raw_stream_)
        {}

        server_response &operator=(const server_response &other) {
            common::response::operator=(other);
            raw_stream_=other.raw_stream_;
            return *this;
        }
        
        void clear();
        
        void set_cookie(const common::cookie &c);
        
        size_t get_content_length() const;
        
        void set_content_type(const std::string &);
        
        const std::string &get_body() const;
        
        inline std::ostream &raw_stream() const {
            return *raw_stream_;
        }
        
        std::ostream &body_stream();
        
        template<typename T>
        void set_body(const T &t, const std::string &content_type=common::content_type<T>::name) {
            set_content_type(content_type);
            body_stream() << t;
        }
        
        bool write_header(std::ostream &os);
        bool write(std::ostream &os);
        
        boost::interprocess::basic_ovectorstream<std::string> raw_body_stream_;
        
        std::ostream *raw_stream_=nullptr;
    };

    inline std::ostream &operator<<(std::ostream &os, server_response &resp) {
        resp.write_header(os);
        if(resp.get_content_length()>0)
            os << resp.body_stream().rdbuf();
        os.flush();
        return os;
    }
}}  // End of namespace fibio::http

#endif
