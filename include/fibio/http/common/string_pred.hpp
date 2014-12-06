//
//  string_pred.hpp
//  fibio-http
//
//  Created by Chen Xu on 14/10/14.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_common_string_pred_hpp
#define fibio_http_common_string_pred_hpp

#include <boost/algorithm/string/predicate.hpp>

namespace fibio { namespace http { namespace common {
    struct starts_with {
        bool operator()(const std::string &s) const {
            return boost::algorithm::starts_with(s, c);
        }
        std::string c;
    };
    struct istarts_with {
        bool operator()(const std::string &s) const {
            return boost::algorithm::istarts_with(s, c);
        }
        std::string c;
    };
    struct ends_with {
        bool operator()(const std::string &s) const {
            return boost::algorithm::ends_with(s, c);
        }
        std::string c;
    };
    struct iends_with {
        bool operator()(const std::string &s) const {
            return boost::algorithm::iends_with(s, c);
        }
        std::string c;
    };
    struct contains {
        bool operator()(const std::string &s) const {
            return boost::algorithm::contains(s, c);
        }
        std::string c;
    };
    struct icontains {
        bool operator()(const std::string &s) const {
            return boost::algorithm::icontains(s, c);
        }
        std::string c;
    };
    struct equals {
        bool operator()(const std::string &s) const {
            return boost::algorithm::equals(s, c);
        }
        std::string c;
    };
    struct iequals {
        bool operator()(const std::string &s) const {
            return boost::algorithm::iequals(s, c);
        }
        std::string c;
    };
}}} // End of namespace fibio::http::common

namespace fibio { namespace http {
    using common::starts_with;
    using common::istarts_with;
    using common::ends_with;
    using common::iends_with;
    using common::contains;
    using common::icontains;
    using common::equals;
    using common::iequals;
}}
#endif
