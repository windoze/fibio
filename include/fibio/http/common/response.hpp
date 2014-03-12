//
//  response.hpp
//  fiberized.io
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_response_hpp
#define fiberized_io_http_common_response_hpp

#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    struct response {
        inline header_value_type get_content_type() const {
            header_map::const_iterator i=headers_.find("content-type");
            if(i==headers_.end()) {
                return "";
            }
            return i->second;
        }
        
        inline void set_content_type(const header_value_type &v) {
            headers_["Content-Type"]=v;
        }
        
        inline void set_content_type(header_value_type &&v) {
            headers_["Content-Type"]=v;
        }
        
        inline http_version get_http_version() const
        { return status_.version_; }
        
        inline void set_http_version(http_version hv)
        { status_.version_=hv; }
        
        inline status_code get_status_code() const
        { return status_.status_; }
        
        inline void set_status_code(status_code sc)
        { status_.status_=sc; }
        
        inline const std::string &get_status_msg() const
        { return status_.message_; }
        
        inline void set_status_msg(const std::string &msg)
        { status_.message_=msg; }
        
        inline void set_status_msg(std::string &&msg)
        { status_.message_.assign(std::move(msg)); }
        
        status_line status_;
        header_map headers_;
    };
}}} // End of namespace fibio::http::common

#endif
