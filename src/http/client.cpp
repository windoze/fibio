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

namespace fibio { namespace http {
    //////////////////////////////////////////////////////////////////////////////////////////
    // client_request
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void client_request::clear() {
        common::request::clear();
        std::string e;
        if (!raw_body_stream_.vector().empty())
            raw_body_stream_.swap_vector(e);
    }
    
    std::ostream &client_request::body_stream() {
        return raw_body_stream_;
    }
    
    size_t client_request::get_content_length() const {
        return raw_body_stream_.vector().size();
    }
    
    void client_request::set_content_type(const std::string &ct) {
        if (ct.empty()) {
            return;
        }
        auto i=headers.find("Content-Type");
        if (i==headers.end()) {
            headers.insert({"Content-Type", ct});
        } else {
            i->second.assign(ct);
        }
    }
    
    void client_request::accept_compressed(bool c) {
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
    
    bool client_request::write_header(std::ostream &os) {
        std::string ka;
        if (keep_alive) {
            ka="keep-alive";
        } else {
            ka="close";
        }
        auto i=headers.find("connection");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Connection", ka));
        } else {
            i->second.assign(ka);
        }
        if (!common::request::write_header(os)) return false;
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    bool client_request::write(std::ostream &os) {
        // Set "content-length"
        auto i=headers.find("content-length");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Content-Length", boost::lexical_cast<std::string>(get_content_length())));
        } else {
            i->second.assign(boost::lexical_cast<std::string>(get_content_length()));
        }
        // Write header
        if (!write_header(os)) return false;
        // Write body
        if (!raw_body_stream_.vector().empty()) {
            os.write(&(raw_body_stream_.vector()[0]), raw_body_stream_.vector().size());
        }
        os.flush();
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // client_response
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void client_response::clear() {
        drop_body();
        common::response::clear();
    }
    
    void client_response::set_auto_decompression(bool c) {
        auto_decompress_=c;
    }
    
    bool client_response::get_auto_decompression() const {
        return auto_decompress_;
    }
    
    bool client_response::read(std::istream &is) {
        clear();
        if (!common::response::read_header(is)) return false;
        
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
    
    void client_response::drop_body() {
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
            BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "HTTP client connect"));
        }
    }
    
    client::client(const std::string &server, int port) {
        boost::system::error_code ec=connect(server, port);
        if (ec) {
            BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "HTTP client connect"));
        }
    }

    client::client(ssl::context &ctx, const std::string &server, const std::string &port)
    : ctx_(&ctx)
    {
        boost::system::error_code ec=connect(ctx, server, port);
        if (ec) {
            BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "HTTP client connect"));
        }
    }
    
    client::client(ssl::context &ctx, const std::string &server, int port)
    : ctx_(&ctx)
    {
        boost::system::error_code ec=connect(ctx, server, port);
        if (ec) {
            BOOST_THROW_EXCEPTION(boost::system::system_error(ec, "HTTP client connect"));
        }
    }
    
    client::~client() {
        if (stream_) {
            if (ctx_) {
                    delete static_cast<ssl::tcp_stream *>(stream_);
            } else {
                delete static_cast<tcp_stream *>(stream_);
            }
        }
    }

    boost::system::error_code client::connect(const std::string &server, const std::string &port) {
        server_=server;
        port_=port;
        stream_=new tcp_stream();
        return static_cast<tcp_stream *>(stream_)->connect(server, port);
    }
    
    boost::system::error_code client::connect(const std::string &server, int port) {
        return connect(server, boost::lexical_cast<std::string>(port));
    }
    
    boost::system::error_code client::connect(ssl::context &ctx, const std::string &server, const std::string &port) {
        server_=server;
        port_=port;
        stream_=new ssl::tcp_stream(ctx);
        return static_cast<ssl::tcp_stream *>(stream_)->connect(server, port);
    }
    
    boost::system::error_code client::connect(ssl::context &ctx, const std::string &server, int port) {
        return connect(ctx, server, boost::lexical_cast<std::string>(port));
    }
    
    void client::disconnect() {
        if (stream_) {
            stream_->close();
        }
    }
    
    void client::set_auto_decompress(bool c) {
        auto_decompress_=c;
    }
    
    bool client::get_auto_decompress() const {
        return auto_decompress_;
    }
    
    bool client::send_request(request &req, response &resp) {
        if (!stream_->is_open() || stream_->eof() || stream_->fail() || stream_->bad()) return false;
        // Make sure there is no pending data in the last response
        resp.clear();
        req.accept_compressed(auto_decompress_);
        resp.set_auto_decompression(auto_decompress_);
        if(!req.write(*stream_)) return false;
        if (!stream_->is_open() || stream_->eof() || stream_->fail() || stream_->bad()) return false;
        //if (!stream_.is_open()) return false;
        return resp.read(*stream_) && (resp.status_code!=http_status_code::INVALID);
    }

    client::request &make_request(client::request &req,
                                  const std::string &url,
                                  const common::header_map &hdr)
    {
        req.clear();
        req.url=url;
        req.method=http_method::GET;
        req.version=http_version::HTTP_1_1;
        req.keep_alive=true;
        req.headers.insert(hdr.begin(), hdr.end());
        return req;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // url_client
    //////////////////////////////////////////////////////////////////////////////////////////
    
    bool url_client::prepare(const std::string &url, const common::header_map &hdr) {
        the_request_.clear();
        common::parsed_url_type purl;
        if(!parse_url(url, purl, false, false))
            return false;
        // TODO: HTTPS
        std::string host=purl.host;
        if (common::iequal()(purl.schema, "http")) {
            if(purl.port==0) {
                purl.port=80;
            } else {
                host+=':';
                host+=boost::lexical_cast<std::string>(purl.port);
            }
            if(!make_client(false, purl.host, purl.port))
                return false;
        } else if (common::iequal()(purl.schema, "https")) {
            if(purl.port==0) {
                purl.port=443;
            } else {
                host+=':';
                host+=boost::lexical_cast<std::string>(purl.port);
            }
            if(!make_client(true, purl.host, purl.port))
                return false;
        } else {
            // ERROR: Unknown protocol
            return false;
        }
        the_request_.url.reserve(url.length());
        the_request_.url=purl.path;
        if(!purl.query.empty()) {
            the_request_.url+='?';
            the_request_.url+=purl.query;
        }
        if(!purl.fragment.empty()) {
            the_request_.url+='?';
            the_request_.url+=purl.fragment;
        }
        the_request_.version=http_version::HTTP_1_1;
        the_request_.keep_alive=true;
        the_request_.headers.insert({"Host", std::move(host)});
        the_request_.headers.insert(hdr.begin(), hdr.end());
        return true;
    }
    
    template<typename Port>
    std::string host_name(bool ssl, const std::string &host, Port port) {
        std::string ret=host;
        if ((ssl && (port==443))
            || (ssl && (port==443)))
        {
            ret += ':';
            ret += boost::lexical_cast<std::string>(port);
        }
        return ret;
    }
    
    bool url_client::make_client(bool ssl, const std::string &host, uint16_t port) {
        try {
            if(the_client_) {
                if ((the_client_->server_!=host) || (the_client_->port_!=boost::lexical_cast<std::string>(port))) {
                    the_response_.drop_body();
                    the_client_->disconnect();
                    if (ssl) {
                        the_client_.reset(new client(settings_.ctx, host, port));
                    } else {
                        the_client_.reset(new client(host, port));
                    }
                }
            } else {
                if (ssl) {
                    the_client_.reset(new client(settings_.ctx, host, port));
                } else {
                    the_client_.reset(new client(host, port));
                }
            }
            return true;
        } catch(boost::system::system_error &e) {
            return false;
        }
    }
    
}}  // End of namespace fibio::http