//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_request_hpp
#define fibio_http_common_request_hpp

#include <list>
#include <iostream>
#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    
    struct parsed_url_type {
        std::string schema;
        std::string host;
        uint16_t port=0;
        std::string path;
        std::string query;
        std::string fragment;
        std::string userinfo;
        std::list<std::string> path_components;
        std::map<std::string, std::string> query_params;
    };
    
    bool parse_url(const std::string url, parsed_url_type &parsed_url, bool parse_path=true, bool parse_query=true);
    
    struct request {
        void clear();
        
        bool read_header(std::istream &is);
        bool write_header(std::ostream &os);
        
        http_method method=http_method::INVALID;
        std::string url;
        http_version version=http_version::INVALID;
        header_map headers;
        size_t content_length=0;
        bool keep_alive=false;
        parsed_url_type parsed_url;
    };
}}} // End of namespace fibio::http::common

#endif
