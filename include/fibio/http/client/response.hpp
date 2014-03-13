//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-11.
//  Copyright (c) 2014年 0d0a.com. All rights reserved.
//

#ifndef fiberized_io_http_client_response_hpp
#define fiberized_io_http_client_response_hpp

#include <memory>
#include <string>
#include <boost/iostreams/restrict.hpp>
#include <fibio/http/common/response.hpp>

namespace fibio { namespace http { namespace client {
    struct response : common::response {
        void clear();
        
        size_t get_content_length() const;
        
        void read(std::istream &is);
        
        inline bool has_body() const {
            return get_content_length()>0 && body_stream_.get();
        }
        
        inline std::istream &body_stream() {
            // TODO: Throw if body stream is not setup
            return *(body_stream_.get());
        }
        
        // Consume and discard body
        void drop_body();
        
        std::unique_ptr<boost::iostreams::restriction<std::istream>> restriction_;
        std::unique_ptr<std::istream> body_stream_;
    };

    inline std::istream &operator>>(std::istream &is, response &v) {
        v.read(is);
        return is;
    }

}}} // End of namespace fibio::http::client


#endif
