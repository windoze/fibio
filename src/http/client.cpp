//
//  client.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-10.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/lexical_cast.hpp>
#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fibio/http/client/client.hpp>

namespace fibio { namespace http { namespace client {
    void request::clear() {
        req_line_.clear();
        headers_.clear();
    }
    
    const std::string &request::get_body() const {
        return body_stream_.vector();
    }
    
    std::ostream &request::body_stream() {
        return body_stream_;
    }
    
    size_t request::get_content_length() const {
        return body_stream_.vector().size();
    }
    
    bool request::write(std::ostream &os) {
        if (!req_line_.write(os)) return false;
        // Make sure there is a Connection header
        set_persistent(get_persistent());
        // Make sure there is a Content-Length header
        headers_["Content-Length"]=boost::lexical_cast<std::string>(get_content_length());
        if (!headers_.write(os)) return false;
        os << "\r\n";
        os << body_stream_.vector();
        os.flush();
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    void response::clear() {
        drop_body();
        status_.clear();
        headers_.clear();
        restriction_.reset();
        body_stream_.reset();
    }
    
    size_t response::get_content_length() const {
        size_t sz=0;
        try {
            sz=boost::lexical_cast<size_t>(headers_.get("content-length", std::string("0")));
        } catch (boost::bad_lexical_cast &e) {
            sz=0;
        }
        return sz;
    }
    
    bool response::read(std::istream &is) {
        if (!status_.read(is)) return false;
        if (!headers_.read(is)) return false;
        if (status_.status_==common::status_code::INVALID) {
            return false;
        }
        if (is.eof()) {
            return false;
        }
        // Setup body stream
        namespace bio = boost::iostreams;
        restriction_.reset(new bio::restriction<std::istream>(is, 0, get_content_length()));
        bio::filtering_istream *in=new bio::filtering_istream;
        in->push(*restriction_);
        body_stream_.reset(in);
        return true;
    }
    
    void response::drop_body() {
        // Discard body content iff body stream exists
        if (body_stream_) {
            while (!body_stream().eof()) {
                char buf[1024];
                body_stream().read(buf, sizeof(buf));
            }
        }
    }
    
    client::client(const std::string &server, const std::string &port) {
        std::error_code ec=connect(server, port);
        if (ec) {
            throw std::system_error(ec, "HTTP client connect");
        }
    }
    
    client::client(const std::string &server, int port) {
        std::error_code ec=connect(server, port);
        if (ec) {
            throw std::system_error(ec, "HTTP client connect");
        }
    }

    std::error_code client::connect(const std::string &server, const std::string &port) {
        stream_.set_connect_timeout(std::chrono::seconds(10));
        stream_.set_write_timeout(std::chrono::seconds(10));
        stream_.set_read_timeout(std::chrono::seconds(10));
        server_=server;
        port_=port;
        return stream_.connect(server, port);
    }
    
    std::error_code client::connect(const std::string &server, int port) {
        return connect(server, boost::lexical_cast<std::string>(port));
    }
    
    void client::disconnect() {
        stream_.close();
    }
    
    bool client::send_request(request &req, response &resp) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        // Make sure there is no pending data in the last response
        resp.clear();
        if(!req.write(stream_)) return false;
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        //if (!stream_.is_open()) return false;
        return resp.read(stream_);
    }
}}} // End of namespace fibio::http::client