//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_request_hpp
#define fiberized_io_http_common_request_hpp

#include <iostream>
#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    struct request {
        void clear();
        
        bool read(std::istream &is);
        bool write(std::ostream &os);
        
        http_method method=http_method::INVALID;
        std::string url;
        http_version version=http_version::INVALID;
        header_map headers;
        size_t content_length=0;
        bool keep_alive=false;
    };
}}} // End of namespace fibio::http::common

#endif
