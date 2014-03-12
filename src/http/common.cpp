//
//  common.cpp
//  fiberized.io
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/common_types.hpp>

namespace fibio { namespace http { namespace common {
    namespace detail {
        const std::map<http_version, std::string> http_version_name_map={
            {http_version::HTTP_0_9, "HTTP/0.9"},
            {http_version::HTTP_1_0, "HTTP/1.0"},
            {http_version::HTTP_1_1, "HTTP/1.1"},
        };
        
        const std::map<std::string, http_version, iless> name_http_version_map={
            {"HTTP/0.9", http_version::HTTP_0_9},
            {"HTTP/1.0", http_version::HTTP_1_0},
            {"HTTP/1.1", http_version::HTTP_1_1},
        };
        
        const std::map<method, std::string> method_name_map={
            {method::DELETE, "DELETE"},
            {method::GET, "GET"},
            {method::HEAD, "HEAD"},
            {method::POST, "POST"},
            {method::PUT, "PUT"},
            /* pathological */
            {method::CONNECT, "CONNECT"},
            {method::OPTIONS, "OPTIONS"},
            {method::TRACE, "TRACE"},
            /* webdav */
            {method::COPY, "COPY"},
            {method::LOCK, "LOCK"},
            {method::MKCOL, "MKCOL"},
            {method::MOVE, "MOVE"},
            {method::PROPFIND, "PROPFIND"},
            {method::PROPPATCH, "PROPPATCH"},
            {method::SEARCH, "SEARCH"},
            {method::UNLOCK, "UNLOCK"},
            /* subversion */
            {method::REPORT, "REPORT"},
            {method::MKACTIVITY, "MKACTIVITY"},
            {method::CHECKOUT, "CHECKOUT"},
            {method::MERGE, "MERGE"},
            /* upnp */
            {method::MSEARCH, "MSEARCH"},
            {method::NOTIFY, "NOTIFY"},
            {method::SUBSCRIBE, "SUBSCRIBE"},
            {method::UNSUBSCRIBE, "UNSUBSCRIBE"},
            /* RFC-5789 */
            {method::PATCH, "PATCH"},
            {method::PURGE, "PURGE"},
        };

        const std::map<std::string, method, iless> name_method_map={
            {"DELETE",  method::DELETE},
            {"GET",     method::GET},
            {"HEAD",    method::HEAD},
            {"POST",    method::POST},
            {"PUT",     method::PUT},
            /* pathological */
            {"CONNECT", method::CONNECT},
            {"OPTIONS", method::OPTIONS},
            {"TRACE",   method::TRACE},
            /* webdav */
            {"COPY",    method::COPY},
            {"LOCK",    method::LOCK},
            {"MKCOL",   method::MKCOL},
            {"MOVE",    method::MOVE},
            {"PROPFIND",method::PROPFIND},
            {"PROPPATCH", method::PROPPATCH},
            {"SEARCH",  method::SEARCH},
            {"UNLOCK",  method::UNLOCK},
            /* subversion */
            {"REPORT",  method::REPORT},
            {"MKACTIVITY",  method::MKACTIVITY},
            {"CHECKOUT",    method::CHECKOUT},
            {"MERGE",   method::MERGE},
            /* upnp */
            {"MSEARCH", method::MSEARCH},
            {"NOTIFY",  method::NOTIFY},
            {"SUBSCRIBE",   method::SUBSCRIBE},
            {"UNSUBSCRIBE", method::UNSUBSCRIBE},
            /* RFC-5789 */
            {"PATCH",   method::PATCH},
            {"PURGE",   method::PURGE},
        };
    }
    
    std::ostream &operator<<(std::ostream &os, const http_version &v) {
        std::map<http_version, std::string>::const_iterator i=detail::http_version_name_map.find(v);
        if (i!=detail::http_version_name_map.end()) {
            os << i->second;
        }
        return os;
    }
    
    std::istream &operator>>(std::istream &is, http_version &v) {
        std::string s;
        is >> s;
        std::map<std::string, http_version, iless>::const_iterator i=detail::name_http_version_map.find(s);
        if (i!=detail::name_http_version_map.end()) {
            v=i->second;
        }
        return is;
    }
    
    std::ostream &operator<<(std::ostream &os, const method &v) {
        std::map<method, std::string>::const_iterator i=detail::method_name_map.find(v);
        if (i!=detail::method_name_map.end()) {
            os << i->second;
        }
        return os;
    }

    std::istream &operator>>(std::istream &is, method &v) {
        std::string s;
        is >> s;
        std::map<std::string, method, iless>::const_iterator i=detail::name_method_map.find(s);
        if (i!=detail::name_method_map.end()) {
            v=i->second;
        }
        return is;
    }
    
    std::ostream &operator<<(std::ostream &os, const status_line &v) {
        os << v.version_ << ' ' << uint(v.status_) << ' ' << v.message_;
        return os;
    }
    
    std::istream &operator>>(std::istream &is, status_line &v) {
        // status_line ::= version code msg
        std::string line;
        std::getline(is, line);
        boost::algorithm::trim(line);
        if (line.empty() || is.eof()) {
            return is;
        }
        auto space=boost::algorithm::is_space();
        std::string::iterator first=line.begin();
        std::string::iterator last=std::find_if(first, line.end(), space);
        if (last==line.end()) {
            // TODO: Error, invalid format
        }
        std::map<std::string, http_version, iless>::const_iterator i=detail::name_http_version_map.find(std::string(first, last));
        if (i==detail::name_http_version_map.end()) {
            // TODO: Error, unknown HTTP version
        }
        v.version_=i->second;
        // Skip adjunct spaces
        first=std::find_if_not(last+1, line.end(), space);
        last=std::find_if(first, line.end(), space);
        v.status_=status_code(boost::lexical_cast<uint16_t>(std::string(first, last)));

        if (last==line.end()) {
            // TODO: No status message, is it an error?
        } else {
            // Skip adjunct spaces
            first=std::find_if_not(last+1, line.end(), space);
            // Last part is status message
            v.message_.assign(first, line.end());
        }
        return is;
    }
    
    std::ostream &operator<<(std::ostream &os, const header_map &v) {
        for (auto &p: v) {
            os << p.first << ": " << p.second << "\r\n";
        }
        return os;
    }
    
    std::istream &operator>>(std::istream &is, header_map &v) {
        std::string line;
        boost::interprocess::basic_vectorstream<std::string> value_stream;
        auto space=boost::algorithm::is_space();
        header_key_type last_key;
        while (true) {
            // TODO: Add max line limit
            std::getline(is, line);
            boost::algorithm::trim_right(line);
            if (!line.empty()) {
                if (space(line[0])) {
                    // Leading space, it is a continuation of last line
                    if (last_key.empty()) {
                        // TODO: Error header format
                    } else {
                        // Compress leading spaces (RFC2616,sec4.2)
                        std::string::iterator i=std::find_if_not(line.begin()+1, line.end(), space);
                        //v[last_key].append(i, line.end());
                        v[last_key]+=" ";
                        v[last_key]+=header_value_type(i, line.end());
                    }
                } else {
                    // key:value
                    std::string::iterator i=std::find(line.begin(), line.end(), ':');
                    header_key_type key(line.begin(), i);
                    header_value_type value(i+1, line.end());
                    boost::algorithm::trim(key);
                    boost::algorithm::trim(value);
                    if (!key.empty() && !value.empty()) {
                        v[key]+=value;
                        last_key=key;
                    }
                }
            } else {
                break;
            }
        }
        return is;
    }
}}} // End of namespace fibio::http::common
