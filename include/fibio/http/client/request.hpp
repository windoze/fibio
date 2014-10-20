//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_client_request_hpp
#define fibio_http_client_request_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/request.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio { namespace http {
    struct client_request : common::request {
        void clear();
        
        size_t get_content_length() const;
        
        void set_content_type(const std::string &);
        
        void accept_compressed(bool);
        
        std::ostream &body_stream();
        
        template<typename T>
        void set_body(const T &t, const std::string &content_type=common::content_type<T>::name) {
            set_content_type(content_type);
            body_stream() << t;
        }

        bool write_header(std::ostream &os);
        bool write(std::ostream &os);

        boost::interprocess::basic_ovectorstream<std::string> raw_body_stream_;
    };
    
    inline std::ostream &operator<<(std::ostream &os, client_request &req) {
        req.write_header(os);
        if(req.get_content_length()>0)
            os << req.body_stream().rdbuf();
        os.flush();
        return os;
    }
}}  // End of namespace fibio::http

#endif
