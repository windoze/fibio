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
    //////////////////////////////////////////////////////////////////////////////////////////
    // request
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void request::clear() {
        // Make sure there is no pending data in the last request
        drop_body();
        common::request::clear();
    }
    
    bool request::accept_compressed() const {
        auto i=headers.find("Accept-Encoding");
        if (i==headers.end()) return false;
        // TODO: Kinda buggy
        return strcasestr(i->second.c_str(), "gzip")!=NULL;
    }
    
    bool request::read(std::istream &is) {
        clear();
        if (!common::request::read(is)) return false;
        if (content_length>0) {
            // Setup body stream
            namespace bio = boost::iostreams;
            restriction_.reset(new bio::restriction<std::istream>(is, 0, content_length));
            bio::filtering_istream *in=new bio::filtering_istream;
            in->push(*restriction_);
            body_stream_.reset(in);
        }
        return true;
    }
    
    void request::drop_body() {
        // Discard body content iff body stream exists
        if (body_stream_) {
            while (!body_stream().eof()) {
                char buf[1024];
                body_stream().read(buf, sizeof(buf));
            }
            body_stream_.reset();
            restriction_.reset();
        }
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // response
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void response::clear() {
        common::response::clear();
        std::string e;
        raw_body_stream_.swap_vector(e);
    }
    
    const std::string &response::get_body() const {
        return raw_body_stream_.vector();
    }
    
    std::ostream &response::body_stream() {
        return raw_body_stream_;
    }
    
    size_t response::get_content_length() const {
        return raw_body_stream_.vector().size();
    }
    
    void response::set_content_type(const std::string &ct) {
        auto i=headers.find("content-type");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Content-Type", ct));
        } else {
            i->second.assign(ct);
        }
    }
    
    bool response::write(std::ostream &os) {
        auto i=headers.find("content-length");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Content-Length", boost::lexical_cast<std::string>(get_content_length())));
        } else {
            i->second.assign(boost::lexical_cast<std::string>(get_content_length()));
        }
        std::string ka;
        if (keep_alive) {
            ka="keep-alive";
        } else {
            ka="close";
        }
        i=headers.find("connection");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Connection", ka));
        } else {
            i->second.assign(ka);
        }
        if (!common::response::write(os)) return false;
        os << raw_body_stream_.vector();
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // server::connection
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void server::connection::close() {
        stream_.close();
    }
    
    bool server::connection::recv(request &req, uint64_t timeout) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        stream_.set_read_timeout(std::chrono::microseconds(timeout));
        return req.read(stream_);
    }
    
    bool server::connection::send(response &resp, uint64_t timeout) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        stream_.set_write_timeout(std::chrono::microseconds(timeout));
        bool ret=resp.write(stream_);
        if (!resp.keep_alive) {
            stream_.close();
            return false;
        }
        return ret;
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // server
    //////////////////////////////////////////////////////////////////////////////////////////
    
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
