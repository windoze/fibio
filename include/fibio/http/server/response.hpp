//
//  response.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_response_hpp
#define fibio_http_server_response_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/response.hpp>

namespace fibio { namespace http {
    struct server_response : common::response {
        typedef server_response this_type;
        server_response()=default;
        
        template<typename T>
        server_response(const T&b)
        { version(http_version::HTTP_1_1).status_code(http_status_code::OK).body(b); }

        server_response(const server_response &other)
        : common::response(other)
        , raw_stream_(other.raw_stream_)
        {}

        server_response &operator=(const server_response &other) {
            common::response::operator=(other);
            raw_stream_=other.raw_stream_;
            return *this;
        }
        
        void clear();
        
        this_type &version(http_version v) { common::response::version=v; return *this; }
        http_version version() const { return common::response::version; }
        this_type &status_code(http_status_code c) { common::response::status_code=c; return *this; }
        http_status_code status_code() const { return common::response::status_code; }
        this_type &keep_alive(bool k) { common::response::keep_alive=k; return *this; }
        bool keep_alive() const { return common::response::keep_alive; }
        
        server_response &header(const std::string &key, const std::string &value);
        server_response &cookie(const common::cookie &c);
        
        server_response &content_type(const std::string &);
        
        const std::string &body() const;
        
        inline std::ostream &raw_stream() const {
            return *raw_stream_;
        }
        
        std::ostream &body_stream();
        
        template<typename T>
        server_response &body(const T &t, const std::string &ct=common::content_type<T>::name) {
            content_type(ct);
            body_stream() << t;
            return *this;
        }
        
        template<typename T>
        std::ostream &operator << (const T &t) {
            body_stream() << t;
            return body_stream();
        }
        
        template<typename T>
        server_response &operator()(const T &t) {
            return body(t);
        }
        
        size_t content_length() const;
        
        bool write_header(std::ostream &os);
        bool write();
        bool write_chunked(std::function<bool(std::ostream &)> body_writer);

        boost::interprocess::basic_ovectorstream<std::string> raw_body_stream_;
        std::ostream *raw_stream_=nullptr;
    };

    inline std::ostream &operator<<(std::ostream &os, server_response &resp) {
        resp.write_header(os);
        if(resp.content_length()>0)
            os << resp.body_stream().rdbuf();
        os.flush();
        return os;
    }
}}  // End of namespace fibio::http

#endif
