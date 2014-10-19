//
//  url_parser.cpp
//  fibio-http
//
//  Created by Chen Xu on 14/10/13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <list>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/iterator_range.hpp>
#include <iostream>
#include <fibio/http/common/url_codec.hpp>
#include "url_parser.hpp"

namespace fibio { namespace http { namespace common {
    typedef std::string::const_iterator string_iter;
    typedef boost::iterator_range<string_iter> string_view;
    typedef std::list<string_view> range_list;

    inline bool is_dot(const string_view &s) {
        return (s.end()-s.begin()==1) && (*(s.begin())=='.');
    }
    
    inline bool is_dot_dot(const string_view &s) {
        return (s.end()-s.begin()==2) && (*(s.begin())=='.') && (*(s.begin()+1)=='.');
    }
    
    bool parse_path_components(const std::string &p, std::list<std::string> &components) {
        std::string path;
        path.reserve(p.length());
        url_decode(p.begin(), p.end(), std::back_insert_iterator<std::string>(path));
        //range_list r;
        std::list<std::string> r;
        boost::split(r, path, boost::is_any_of("/"), boost::token_compress_on);
        
        for (auto &i : r) {
            if (i.empty()) {
                continue;
            }
            if (is_dot_dot(i)) {
                if (components.empty()) {
                    // ERROR: out of root directory
                    return false;
                }
                components.pop_back();
            } else if (!is_dot(i)) {
                // Skip '.'
                components.emplace_back(i.begin(), i.end());
            }
        }
        return true;
    }
    
    bool parse_query_string(const std::string &query, std::map<std::string, std::string> &parameters) {
        range_list r;
        boost::split(r, query, boost::is_any_of("&"), boost::token_compress_on);
        
        for (auto &i : r) {
            if (i.empty()) {
                continue;
            }
            auto eq=std::find(i.begin(), i.end(), '=');
            if (eq==i.begin()) {
                // Invalid format "=XXX"
                return false;
            }
            std::string key;
            key.reserve(eq-i.begin());
            std::string value;
            url_decode(i.begin(), eq, std::back_insert_iterator<std::string>(key));
            if (eq!=i.end()) {
                ++eq;
                value.reserve(i.end()-eq);
                url_decode(eq, i.end(), std::back_insert_iterator<std::string>(value));
            }
            parameters.insert({std::move(key), std::move(value)});
        }
        return true;
    }
}}} // End of namespace fibio::http::common