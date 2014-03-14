//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_request_hpp
#define fiberized_io_http_common_request_hpp

#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    struct request {
        inline header_value_type get_content_type() const {
            return headers_["Content-Type"];
        }
        
        inline void set_content_type(const header_value_type &v) {
            headers_["Content-Type"]=v;
        }
        
        inline void set_content_type(header_value_type &&v) {
            headers_["Content-Type"]=v;
        }
        
        inline header_value_type get_host() const {
            return headers_["host"];
        }
        
        inline void set_host(const header_value_type &v) {
            headers_["Host"]=v;
        }
        
        inline void set_host(header_value_type &&v) {
            headers_["Host"]=v;
        }

        inline bool get_persistent() const {
            std::string conn=headers_["connection"];
            if (req_line_.version_<http_version::HTTP_1_1) {
                // Default to non-persistent
                return iequal()(conn, "keep-alive");
            }
            // Default to persistent
            return conn.empty() || iequal()(conn, "keep-alive");
        }
        
        inline void set_persistent(bool v) {
            headers_["Connection"]= v ? "keep-alive" : "close";
        }

        inline method get_method() const
        { return req_line_.method_; }
        
        inline void set_method(method m)
        { req_line_.method_=m; }
        
        inline std::string get_url() const
        { return req_line_.url_; }
        
        inline void set_url(const std::string &u)
        { req_line_.url_=u; }
        
        inline void set_url(std::string &&u)
        { req_line_.url_=std::move(u); }
        
        inline http_version get_http_version() const
        { return req_line_.version_; }
        
        inline void set_http_version(http_version hv)
        { req_line_.version_=hv; }
        
        request_line req_line_;
        header_map headers_;
    };
}}} // End of namespace fibio::http::common

#endif
