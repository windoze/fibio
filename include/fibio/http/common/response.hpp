//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_response_hpp
#define fibio_http_common_response_hpp

#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    struct response {
        void clear();
        
        const std::string &header(const std::string &name) const;
        void add_header(const std::string &name, const std::string &value);
        void set_header(const std::string &name, const std::string &value);
        
        bool read_header(std::istream &is);
        bool write_header(std::ostream &os);
        
        http_version version=http_version::INVALID_VERSION;
        http_status_code status_code=http_status_code::INVALID_STATUS;
        std::string status_message;
        header_map headers;
        size_t content_length=0;
        bool keep_alive=false;
    };
}}} // End of namespace fibio::http::common

#endif
