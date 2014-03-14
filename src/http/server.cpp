//
//  server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/lexical_cast.hpp>
#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <fibio/http/server/server.hpp>

namespace fibio { namespace http { namespace server {
    void request::clear() {
        // Make sure there is no pending data in the last request
        drop_body();
        req_line_.clear();
        headers_.clear();
        restriction_.reset();
        body_stream_.reset();
    }
    
    size_t request::get_content_length() const {
        size_t sz=0;
        try {
            sz=boost::lexical_cast<size_t>(headers_.get("content-length", std::string("0")));
        } catch (boost::bad_lexical_cast &e) {
            sz=0;
        }
        return sz;
    }
    
    bool request::read(std::istream &is) {
        if (!req_line_.read(is)) return false;
        if (!headers_.read(is)) return false;
        if (req_line_.method_==common::method::INVALID) {
            return false;
        }
        if (is.eof() || is.fail() || is.bad()) return false;
        // Setup body stream
        namespace bio = boost::iostreams;
        restriction_.reset(new bio::restriction<std::istream>(is, 0, get_content_length()));
        bio::filtering_istream *in=new bio::filtering_istream;
        in->push(*restriction_);
        body_stream_.reset(in);
        return true;
    }
    
    void request::drop_body() {
        // Discard body content iff body stream exists
        if (body_stream_) {
            while (!body_stream().eof()) {
                char buf[1024];
                body_stream().read(buf, sizeof(buf));
            }
        }
    }
    
    void response::clear() {
        status_.clear();
        headers_.clear();
    }
    
    const std::string &response::get_body() const {
        return body_stream_.vector();
    }
    
    std::ostream &response::body_stream() {
        return body_stream_;
    }
    
    size_t response::get_content_length() const {
        return body_stream_.vector().size();
    }
    
    bool response::write(std::ostream &os) {
        if (!status_.write(os)) return false;
        // Make sure there is a Connection header
        set_persistent(get_persistent());
        // Make sure there is a Content-Length header
        headers_["Content-Length"]=boost::lexical_cast<std::string>(get_content_length());
        if (!headers_.write(os)) return false;
        os << "\r\n";
        if (os.eof() || os.fail() || os.bad()) return false;
        os << body_stream_.vector();
        os.flush();
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    server::connection::connection(const server::connection &other)
    : stream_(std::move(other.stream_))
    , host_(other.host_)
    {}
    
    bool server::connection::recv(request &req, uint64_t timeout) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        req.clear();
        stream_.set_read_timeout(std::chrono::microseconds(timeout));
        return req.read(stream_);
    }
    
    bool server::connection::send(response &resp, uint64_t timeout) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        stream_.set_write_timeout(std::chrono::microseconds(timeout));
        if (/*!host_.empty()*/ 0) {
            common::header_map::const_iterator i=resp.headers_.find("host");
            // Make sure there is a Host header
            if (i==resp.headers_.end()) {
                resp.set_host(host_);
            }
        }
        bool ret=resp.write(stream_);
        if (!resp.get_persistent()) {
            stream_.close();
            return false;
        }
        return ret;
    }
    
    server::server(const io::tcp::endpoint &ep, const std::string &host)
    : host_(host)
    , acceptor_(io::listen(ep))
    {}
    
    std::error_code server::accept(server::connection &sc, uint64_t timeout) {
        std::error_code ec;
        sc.stream_.stream_descriptor()=io::accept(acceptor_, timeout, ec);
        if (!ec) {
            ;
            sc.host_=host_;
        }
        return ec;
    }
    
    void server::close() {
        acceptor_.close();
    }
}}} // End of namespace fibio::http::server
