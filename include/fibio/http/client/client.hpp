//
//  client.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-11.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_client_client_hpp
#define fibio_http_client_client_hpp

#include <string>
#include <functional>
#include <fibio/stream/iostream.hpp>
#include <fibio/stream/ssl.hpp>
#include <fibio/http/client/request.hpp>
#include <fibio/http/client/response.hpp>
#include <fibio/http/common/url_codec.hpp>

namespace fibio {
namespace http {

struct client
{
    typedef fibio::http::client_request request;
    typedef fibio::http::client_response response;

    client() = default;

    client(const std::string& server, const std::string& port);

    client(const std::string& server, int port = 80);

    client(ssl::context& ctx, const std::string& server, const std::string& port);

    client(ssl::context& ctx, const std::string& server, int port = 443);

    ~client();

    boost::system::error_code connect(const std::string& server, const std::string& port);

    boost::system::error_code connect(const std::string& server, int port = 80);

    boost::system::error_code
    connect(ssl::context& ctx, const std::string& server, const std::string& port);

    boost::system::error_code connect(ssl::context& ctx, const std::string& server, int port = 443);

    void disconnect();

    bool is_open() const
    {
        return stream_ && stream_->is_open() && !stream_->eof() && !stream_->fail()
               && !stream_->bad();
    }

    void auto_decompress(bool c);

    bool auto_decompress() const;

    bool send_request(request& req, response& resp);

    bool send_chunked_request(request& req,
                              std::function<bool(std::ostream&)> body_writer,
                              response& resp);

    std::string server_;
    std::string port_;
    ssl::context* ctx_;
    stream::closable_stream* stream_;
    bool auto_decompress_ = false;
};

// GET
inline client::request& make_request(client::request& req,
                                     const std::string& url,
                                     const common::header_map& hdr = common::header_map())
{
    req.clear();
    req.url(url).method(http_method::GET).version(http_version::HTTP_1_1).keep_alive(true);
    req.headers.insert(hdr.begin(), hdr.end());
    return req;
}

// POST
template <typename T>
client::request& make_request(client::request& req,
                              const std::string& url,
                              const T& body,
                              const common::header_map& hdr = common::header_map())
{
    req.clear();
    req.url(url).method(http_method::POST).version(http_version::HTTP_1_1).keep_alive(true);
    req.headers.insert(hdr.begin(), hdr.end());
    // Default content type for HTML Forms
    req.content_type("application/x-www-form-urlencoded");
    // Write URL encoded body into body stream
    url_encode(body, std::ostreambuf_iterator<char>(req.body_stream()));
    return req;
}

struct url_client
{
    struct settings
    {
        ssl::context ctx = ssl::context(ssl::context::tlsv1_client);
        int max_redirection = 0;
    };

    url_client() = default;

    url_client(settings&& s) : settings_(std::move(s)) {}

    inline client::response& request(const std::string& url,
                                     const common::header_map& hdr = common::header_map(),
                                     unsigned max_redirection = 50)
    {
        the_request_.clear();
        if (prepare(url)) {
            the_request_.method(http_method::GET);
            if (!hdr.empty()) the_request_.headers.insert(hdr.begin(), hdr.end());
            the_client_->send_request(the_request_, the_response_);
        }
        if (max_redirection > 0) {
            auto i = the_response_.headers.find("location");
            if (static_cast<uint16_t>(the_response_.status_code) / 100 == 3
                && i != the_response_.headers.end()) {
                // 3xx redirect with "Location" header
                return request(i->second, hdr, max_redirection - 1);
            }
        }
        return the_response_;
    }

    template <typename T>
    client::response& request(const std::string& url,
                              const T& body,
                              const common::header_map& hdr = common::header_map(),
                              unsigned max_redirection = std::numeric_limits<unsigned>::max())
    {
        the_request_.clear();
        if (prepare(url)) {
            the_request_.method(http_method::POST)
                .content_type("application/x-www-form-urlencoded");
            // Write URL encoded body into body stream
            url_encode(body, std::ostreambuf_iterator<char>(the_request_.body_stream()));
            if (!hdr.empty()) the_request_.headers.insert(hdr.begin(), hdr.end());
            the_client_->send_request(the_request_, the_response_);
            auto i = the_response_.headers.find("location");
            if (max_redirection > 0) {
                if (static_cast<uint16_t>(the_response_.status_code) / 100 == 3
                    && i != the_response_.headers.end()) {
                    // 3xx redirect with "Location" header
                    if (the_response_.status_code == http_status_code::TEMPORARY_REDIRECT) {
                        // 307 needs to resend request with original method
                        return request(i->second, body, hdr, max_redirection - 1);
                    } else {
                        // Other 3xx uses "POST-Redirection-GET"
                        return request(i->second, hdr, max_redirection - 1);
                    }
                }
            }
        }
        return the_response_;
    }

    inline client::response& chunked_request(const std::string& url,
                                             std::function<bool(std::ostream&)> body_writer,
                                             const common::header_map& hdr = common::header_map(),
                                             unsigned max_redirection
                                             = std::numeric_limits<unsigned>::max())
    {
        the_request_.clear();
        if (prepare(url)) {
            the_request_.method(http_method::POST);
            if (!hdr.empty()) the_request_.headers.insert(hdr.begin(), hdr.end());
            the_client_->send_chunked_request(the_request_, body_writer, the_response_);
            auto i = the_response_.headers.find("location");
            if (max_redirection > 0) {
                if (static_cast<uint16_t>(the_response_.status_code) / 100 == 3
                    && i != the_response_.headers.end()) {
                    // 3xx redirect with "Location" header
                    if (the_response_.status_code == http_status_code::TEMPORARY_REDIRECT) {
                        // 307 needs to resend request with original method
                        return chunked_request(i->second, body_writer, hdr, max_redirection - 1);
                    } else {
                        // Other 3xx uses "POST-Redirection-GET"
                        return request(i->second, hdr, max_redirection - 1);
                    }
                }
            }
        }
        return the_response_;
    }

private:
    bool prepare(const std::string& url, const common::header_map& hdr = common::header_map());

    bool make_client(bool ssl, const std::string& host, uint16_t port);

    std::unique_ptr<client> the_client_;
    client::request the_request_;
    client::response the_response_;
    // Default ssl context
    settings settings_;
};

} // End of namespace http
} // End of namespace fibio

#endif
