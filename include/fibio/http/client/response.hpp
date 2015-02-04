//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-11.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_client_response_hpp
#define fibio_http_client_response_hpp

#include <memory>
#include <string>
#include <boost/iostreams/restrict.hpp>
#include <fibio/http/common/response.hpp>

namespace fibio { namespace http {
    struct client_response : common::response {
        void clear();
        
        bool read(std::istream &is);
        
        inline bool has_body() const {
            return content_length>0 && body_stream_.get();
        }
        
        inline std::istream &body_stream() {
            // TODO: Throw if body stream is not setup
            return *(body_stream_.get());
        }
        
        // Consume and discard body
        void drop_body();
        
#ifdef HAVE_ZLIB
        void auto_decompression(bool c);
        bool auto_decompression() const;
        bool auto_decompress_=false;
#endif
        
        std::unique_ptr<boost::iostreams::restriction<std::istream>> restriction_;
        std::unique_ptr<std::istream> body_stream_;
    };

    inline std::istream &operator>>(std::istream &is, client_response &v) {
        v.read(is);
        return is;
    }

    inline std::ostream &operator<<(std::ostream &os, client_response &resp) {
        resp.write_header(os);
        if(resp.has_body())
            os << resp.body_stream().rdbuf();
        os.flush();
        return os;
    }
}}  // End of namespace fibio::http


#endif
