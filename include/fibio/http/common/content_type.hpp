//
//  content_type.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-11.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_common_content_type_hpp
#define fiberized_io_http_common_content_type_hpp

namespace fibio { namespace http { namespace common {
    template<typename T>
    struct content_type {
        static constexpr const char *name="text/plain";
    };
    /*
     template<>
     struct content_type<data::JSON> {
        static constexpr const char *name="text/json";
     };
     
     template<>
     struct content_type<data::XML> {
        static constexpr const char *name="text/xml";
     };
     */
}}} // End of namespace fibio::http::common

#endif
