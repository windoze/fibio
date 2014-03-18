//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_response_hpp
#define fiberized_io_http_common_response_hpp

#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    struct response {
        void clear();
        
        bool read(std::istream &is);
        bool write(std::ostream &os);
        
        http_version version=http_version::INVALID;
        http_status_code status_code=http_status_code::INVALID;
        std::string status_message;
        header_map headers;
        size_t content_length=0;
        bool keep_alive=false;
    };
}}} // End of namespace fibio::http::common

#endif
