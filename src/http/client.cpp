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
#include <boost/iostreams/filter/gzip.hpp>
#include <fibio/http/client/client.hpp>

namespace fibio { namespace http { namespace client {
    //////////////////////////////////////////////////////////////////////////////////////////
    // request
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void request::clear() {
        common::request::clear();
        std::string e;
        raw_body_stream_.swap_vector(e);
    }
    
    std::ostream &request::body_stream() {
        return raw_body_stream_;
    }
    
    size_t request::get_content_length() const {
        return raw_body_stream_.vector().size();
    }
    
    void request::accept_compressed(bool c) {
        if (c) {
            // Support gzip only for now
            common::header_map::iterator i=headers.find("Accept-Encoding");
            if (i==headers.end()) {
                headers.insert(std::make_pair("Accept-Encoding", "gzip"));
            } else {
                i->second="gzip";
            }
        } else {
            headers.erase("Accept-Encoding");
        }
    }
    
    bool request::write(std::ostream &os) {
        if (!common::request::write(os)) return false;
        os.write(&(raw_body_stream_.vector()[0]), raw_body_stream_.vector().size());
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // response
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void response::clear() {
        drop_body();
        common::response::clear();
    }
    
    void response::set_auto_decompression(bool c) {
        auto_decompress_=c;
    }
    
    bool response::get_auto_decompression() const {
        return auto_decompress_;
    }
    
    bool response::read(std::istream &is) {
        clear();
        if (!common::response::read(is)) return false;
        
        if (content_length>0) {
            // Setup body stream
            namespace bio = boost::iostreams;
            bio::filtering_istream *in=new bio::filtering_istream;
            if (auto_decompress_) {
                // Support gzip only for now
                auto i=headers.find("Content-Encoding");
                if (i!=headers.end() && common::iequal()(i->second, std::string("gzip"))) {
                    in->push(boost::iostreams::gzip_decompressor());
                }
            }
            restriction_.reset(new bio::restriction<std::istream>(is, 0, content_length));
            in->push(*restriction_);
            body_stream_.reset(in);
        }
        return true;
    }
    
    void response::drop_body() {
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
    // client
    //////////////////////////////////////////////////////////////////////////////////////////
    
    client::client(const std::string &server, const std::string &port) {
        boost::system::error_code ec=connect(server, port);
        if (ec) {
            throw boost::system::system_error(ec, "HTTP client connect");
        }
    }
    
    client::client(const std::string &server, int port) {
        boost::system::error_code ec=connect(server, port);
        if (ec) {
            throw boost::system::system_error(ec, "HTTP client connect");
        }
    }

    boost::system::error_code client::connect(const std::string &server, const std::string &port) {
        server_=server;
        port_=port;
        return stream_.connect(server, port);
    }
    
    boost::system::error_code client::connect(const std::string &server, int port) {
        return connect(server, boost::lexical_cast<std::string>(port));
    }
    
    void client::disconnect() {
        stream_.close();
    }
    
    void client::set_auto_decompress(bool c) {
        auto_decompress_=c;
    }
    
    bool client::get_auto_decompress() const {
        return auto_decompress_;
    }
    
    bool client::send_request(request &req, response &resp) {
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        // Make sure there is no pending data in the last response
        resp.clear();
        req.accept_compressed(auto_decompress_);
        resp.set_auto_decompression(auto_decompress_);
        if(!req.write(stream_)) return false;
        if (!stream_.is_open() || stream_.eof() || stream_.fail() || stream_.bad()) return false;
        //if (!stream_.is_open()) return false;
        return resp.read(stream_);
    }
}}} // End of namespace fibio::http::client