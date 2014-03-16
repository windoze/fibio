//
//  common.cpp
//  fibio
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
        
        const std::map<status_code, std::string> status_msg_map={
            {status_code::CONTINUE                        , "Continue"},
            {status_code::SWITCHING_PROTOCOLS             , "Switching Protocols"},
            {status_code::OK                              , "OK"},
            {status_code::CREATED                         , "Created"},
            {status_code::ACCEPTED                        , "Accepted"},
            {status_code::NON_AUTHORITATIVE_INFORMATION   , "Non-Authoritative Information"},
            {status_code::NO_CONTENT                      , "No Content"},
            {status_code::RESET_CONTENT                   , "Reset Content"},
            {status_code::PARTIAL_CONTENT                 , "Partial Content"},
            {status_code::MULTIPLE_CHOICES                , "Multiple Choices"},
            {status_code::MOVED_PERMANENTLY               , "Moved Permanently"},
            {status_code::FOUND                           , "Found"},
            {status_code::SEE_OTHER                       , "See Other"},
            {status_code::NOT_MODIFIED                    , "Not Modified"},
            {status_code::USE_PROXY                       , "Use Proxy"},
            //status_code::UNUSED                         , "(Unused)"},
            {status_code::TEMPORARY_REDIRECT              , "Temporary Redirect"},
            {status_code::BAD_REQUEST                     , "Bad Request"},
            {status_code::UNAUTHORIZED                    , "Unauthorized"},
            {status_code::PAYMENT_REQUIRED                , "Payment Required"},
            {status_code::FORBIDDEN                       , "Forbidden"},
            {status_code::NOT_FOUND                       , "Not Found"},
            {status_code::METHOD_NOT_ALLOWED              , "Method Not Allowed"},
            {status_code::NOT_ACCEPTABLE                  , "Not Acceptable"},
            {status_code::PROXY_AUTHENTICATION_REQUIRED   , "Proxy Authentication Required"},
            {status_code::REQUEST_TIMEOUT                 , "Request Timeout"},
            {status_code::CONFLICT                        , "Conflict"},
            {status_code::GONE                            , "Gone"},
            {status_code::LENGTH_REQUIRED                 , "Length Required"},
            {status_code::PRECONDITION_FAILED             , "Precondition Failed"},
            {status_code::REQUEST_ENTITY_TOO_LARGE        , "Request Entity Too Large"},
            {status_code::REQUEST_URI_TOO_LONG            , "Request-URI Too Long"},
            {status_code::UNSUPPORTED_MEDIA_TYPE          , "Unsupported Media Type"},
            {status_code::REQUESTED_RANGE_NOT_SATISFIABLE , "Requested Range Not Satisfiable"},
            {status_code::EXPECTATION_FAILED              , "Expectation Failed"},
            {status_code::UPGRADE_REQUIRED                , "Upgrade Required"},
            {status_code::PRECONDITION_REQUIRED           , "Precondition Required"},
            {status_code::TOO_MANY_REQUESTS               , "Too Many Requests"},
            {status_code::REQUEST_HEADER_FIELDS_TOO_LARGE , "Request Header Fields Too Large"},
            {status_code::INTERNAL_SERVER_ERROR           , "Internal Server Error"},
            {status_code::NOT_IMPLEMENTED                 , "Not Implemented"},
            {status_code::BAD_GATEWAY                     , "Bad Gateway"},
            {status_code::SERVICE_UNAVAILABLE             , "Service Unavailable"},
            {status_code::GATEWAY_TIMEOUT                 , "Gateway Timeout"},
            {status_code::HTTP_VERSION_NOT_SUPPORTED      , "HTTP Version Not Supported"},
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
        } else {
            v=http_version::INVALID;
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
        } else {
            v=method::INVALID;
        }
        return is;
    }
    
    void request_line::clear() {
        method_=method::INVALID;
        url_.clear();
        version_=http_version::INVALID;
    }
    
    //std::ostream &operator<<(std::ostream &os, const request_line &v) {
    bool request_line::write(std::ostream &os) const {
        os << method_ << ' ' << url_ << ' ' << version_ << "\r\n";
        return true;
    }
    
    //std::istream &operator>>(std::istream &is, request_line &v) {
    bool request_line::read(std::istream &is) {
        std::string line;
        std::getline(is, line);
        boost::algorithm::trim(line);
        if (line.empty() || is.eof()) {
            return false;
        }
        // Method
        auto space=boost::algorithm::is_space();
        std::string::iterator first=line.begin();
        std::string::iterator last=std::find_if(first, line.end(), space);
        if (last==line.end()) {
            return false;
        }
        std::map<std::string, method, iless>::const_iterator i=detail::name_method_map.find(std::string(first, last));
        if (i==detail::name_method_map.end()) {
            // TODO: Error, unknown method
            method_=method::INVALID;
            return false;
        }
        method_=i->second;
        // URL
        // Skip adjunct spaces
        first=std::find_if_not(last+1, line.end(), space);
        last=std::find_if(first, line.end(), space);
        url_.assign(first, last);
        // HTTP Version
        first=std::find_if_not(last+1, line.end(), space);
        if (first==line.end()) {
            // No version specified, default to HTTP 1.0
            version_=http_version::HTTP_1_0;
        } else {
            std::map<std::string, http_version, iless>::const_iterator i=detail::name_http_version_map.find(std::string(first, line.end()));
            if (i==detail::name_http_version_map.end()) {
                // TODO: Error, unknown HTTP version
                version_=http_version::INVALID;
                return false;
            }
            version_=i->second;
        }
        return true;
    }
    
    void status_line::clear() {
        version_=http_version::INVALID;
        status_=status_code::INVALID;
        message_.clear();
    }
    
    //std::ostream &operator<<(std::ostream &os, const status_line &v) {
    bool status_line::write(std::ostream &os) const {
        os << version_ << ' ' << uint(status_) << ' ' << message_ << "\r\n";
        return true;
    }
    
    //std::istream &operator>>(std::istream &is, status_line &v) {
    bool status_line::read(std::istream &is) {
        // status_line ::= version code msg
        std::string line;
        std::getline(is, line);
        boost::algorithm::trim(line);
        if (line.empty() || is.eof()) {
            return false;
        }
        auto space=boost::algorithm::is_space();
        std::string::iterator first=line.begin();
        std::string::iterator last=std::find_if(first, line.end(), space);
        if (last==line.end()) {
            // TODO: Error, invalid format
            return false;
        }
        std::map<std::string, http_version, iless>::const_iterator i=detail::name_http_version_map.find(std::string(first, last));
        if (i==detail::name_http_version_map.end()) {
            // TODO: Error, unknown HTTP version
            version_=http_version::INVALID;
            return false;
        }
        version_=i->second;
        if (version_!=http_version::HTTP_0_9 && version_!=http_version::HTTP_1_0 && version_!=http_version::HTTP_1_1) {
            //std::cout << "XXXXxxXXX" << std::endl;
            return false;
        }
        // Skip adjunct spaces
        first=std::find_if_not(last+1, line.end(), space);
        last=std::find_if(first, line.end(), space);
        try {
            status_=status_code(boost::lexical_cast<uint16_t>(std::string(first, last)));
        } catch(boost::bad_lexical_cast &e) {
            status_=status_code::INVALID;
            return false;
        }

        if (last==line.end()) {
            // TODO: No status message, is it an error?
        } else {
            // Skip adjunct spaces
            first=std::find_if_not(last+1, line.end(), space);
            // Last part is status message
            message_.assign(first, line.end());
        }
        return true;
    }
    
    void status_line::set_status_code(status_code sc, const std::string &msg) {
        status_=sc;
        if (msg.empty()) {
            std::map<status_code, std::string>::const_iterator i=detail::status_msg_map.find(sc);
            if (i!=detail::status_msg_map.end()) {
                message_=i->second;
            }
        } else {
            message_=msg;
        }
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
