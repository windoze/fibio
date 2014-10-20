//
//  url_parser.h
//  fibio-http
//
//  Created by Chen Xu on 14/10/13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_url_parser_hpp
#define fibio_http_url_parser_hpp

#include <map>
#include <list>
#include <string>

namespace fibio { namespace http { namespace common {
    bool parse_path_components(const std::string &path, std::list<std::string> &components);
    bool parse_query_string(const std::string &query, std::map<std::string, std::string> &parameters);
}}} // End of namespace fibio::http::common

#endif /* defined(fibio_http_url_parser_hpp) */
