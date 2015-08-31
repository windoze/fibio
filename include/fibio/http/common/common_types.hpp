//
//  common_types.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_common_types_hpp
#define fibio_http_common_common_types_hpp

#include <cstdint>
#include <string>
#include <map>
#include <list>
#include <chrono>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio { namespace http { namespace common {
    enum class http_version : uint16_t {
        INVALID_VERSION=0x0000,
        HTTP_0_9=0x0009,
        HTTP_1_0=0x0100,
        HTTP_1_1=0x0101,
    };
    
    enum class http_method : uint16_t {
        // Name conflict
        DELETE_=0,
        GET=1,
        HEAD=2,
        POST=3,
        PUT=4,
        /* pathological */
        CONNECT=5,
        OPTIONS=6,
        TRACE=7,
        /* webdav */
        COPY=8,
        LOCK=9,
        MKCOL=10,
        MOVE=11,
        PROPFIND=12,
        PROPPATCH=13,
        SEARCH=14,
        UNLOCK=15,
        /* subversion */
        REPORT=16,
        MKACTIVITY=17,
        CHECKOUT=18,
        MERGE=19,
        /* upnp */
        MSEARCH=20,
        NOTIFY=21,
        SUBSCRIBE=22,
        UNSUBSCRIBE=23,
        /* RFC-5789 */
        PATCH=24,
        PURGE=25,
        INVALID_METHOD=0xFFFF,
    };
    
    enum class http_status_code : uint16_t {
        INVALID_STATUS                  =0,
        CONTINUE                        =100,
        SWITCHING_PROTOCOLS             =101,
        OK                              =200,
        CREATED                         =201,
        ACCEPTED                        =202,
        NON_AUTHORITATIVE_INFORMATION   =203,
        NO_CONTENT                      =204,
        RESET_CONTENT                   =205,
        PARTIAL_CONTENT                 =206,
        MULTIPLE_CHOICES                =300,
        MOVED_PERMANENTLY               =301,
        FOUND                           =302,
        SEE_OTHER                       =303,
        NOT_MODIFIED                    =304,
        USE_PROXY                       =305,
        //UNUSED                        =306,
        TEMPORARY_REDIRECT              =307,
        BAD_REQUEST                     =400,
        UNAUTHORIZED                    =401,
        PAYMENT_REQUIRED                =402,
        FORBIDDEN                       =403,
        NOT_FOUND                       =404,
        METHOD_NOT_ALLOWED              =405,
        NOT_ACCEPTABLE                  =406,
        PROXY_AUTHENTICATION_REQUIRED   =407,
        REQUEST_TIMEOUT                 =408,
        CONFLICT                        =409,
        GONE                            =410,
        LENGTH_REQUIRED                 =411,
        PRECONDITION_FAILED             =412,
        REQUEST_ENTITY_TOO_LARGE        =413,
        REQUEST_URI_TOO_LONG            =414,
        UNSUPPORTED_MEDIA_TYPE          =415,
        REQUESTED_RANGE_NOT_SATISFIABLE =416,
        EXPECTATION_FAILED              =417,
        UPGRADE_REQUIRED                =426,
        PRECONDITION_REQUIRED           =428,
        TOO_MANY_REQUESTS               =429,
        REQUEST_HEADER_FIELDS_TOO_LARGE =431,
        INTERNAL_SERVER_ERROR           =500,
        NOT_IMPLEMENTED                 =501,
        BAD_GATEWAY                     =502,
        SERVICE_UNAVAILABLE             =503,
        GATEWAY_TIMEOUT                 =504,
        HTTP_VERSION_NOT_SUPPORTED      =505,
    };
    
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
    
    typedef std::string header_key_type;
    typedef std::string header_value_type;
    
    struct iless {
        inline bool operator()(const header_key_type &lhs, const header_key_type &rhs) const
        { return boost::algorithm::lexicographical_compare(lhs, rhs, op_); }
        boost::algorithm::is_iless op_;
    };
    
    struct iequal {
        inline bool operator()(const header_key_type &lhs, const header_key_type &rhs) const
        { return boost::iequals(lhs, rhs); }
    };

    typedef std::multimap<header_key_type, header_value_type, iless> header_map;
    
    std::string base64_encode(const char *s, size_t sz);
    std::string base64_encode(const std::string &s);
    std::string base64_decode(const std::string& s);
}}} // End of namespace fibio::http::common

namespace fibio { namespace http {
    using common::http_version;
    using common::http_method;
    using common::http_status_code;
}}  // End of namespace fibio::http


#endif
