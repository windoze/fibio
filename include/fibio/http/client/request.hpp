//
//  request.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_client_request_hpp
#define fibio_http_client_request_hpp

#include <string>
#include <boost/interprocess/streams/vectorstream.hpp>
#include <fibio/http/common/request.hpp>
#include <fibio/http/common/content_type.hpp>

namespace fibio {
namespace http {

struct client_request : common::request
{
    typedef client_request this_type;

    void clear();

    size_t content_length() const;

    this_type& url(const std::string& u)
    {
        common::request::url = u;
        return *this;
    }

    const std::string& url() const { return common::request::url; }

    std::string& url() { return common::request::url; }

    this_type& version(http_version v)
    {
        common::request::version = v;
        return *this;
    }

    http_version version() const { return common::request::version; }

    this_type& method(http_method m)
    {
        common::request::method = m;
        return *this;
    }

    http_method method() const { return common::request::method; }

    this_type& keep_alive(bool k)
    {
        common::request::keep_alive = k;
        return *this;
    }

    bool keep_alive() const { return common::request::keep_alive; }

    this_type& content_type(const std::string&);

    this_type& header(const std::string& key, const std::string& value);

    this_type& accept_compressed(bool);

    std::ostream& body_stream();

    template <typename T>
    this_type& body(const T& t, const std::string& ct = common::content_type<T>::name)
    {
        content_type(ct);
        body_stream() << t;
        return *this;
    }

    template <typename T>
    std::ostream& operator<<(const T& t)
    {
        body_stream() << t;
        return body_stream();
    }

    template <typename T>
    this_type& operator()(const T& t)
    {
        return body(t);
    }

    bool write_header(std::ostream& os);

    bool write(std::ostream& os);

    bool write_chunked(std::ostream& os, std::function<bool(std::ostream&)> body_writer);

    boost::interprocess::basic_ovectorstream<std::string> raw_body_stream_;
};

inline std::ostream& operator<<(std::ostream& os, client_request& req)
{
    req.write_header(os);
    if (req.content_length() > 0) os << req.body_stream().rdbuf();
    os.flush();
    return os;
}

} // End of namespace http
} // End of namespace fibio

#endif
