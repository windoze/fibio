//
//  server.cpp
//  fibio
//
//  Created by Chen Xu on 14-3-13.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <fibio/future.hpp>
#include <fibio/http/server/server.hpp>

namespace fibio { namespace http {
    namespace detail {
        typedef fibio::http::server_request request;
        typedef fibio::http::server_response response;
        typedef boost::asio::basic_waitable_timer<std::chrono::steady_clock> watchdog_timer_t;
        
        template<typename Stream>
        struct stream_traits {};
        
        template<>
        struct stream_traits<tcp_stream> {
            typedef tcp_stream stream_type;
            typedef tcp_stream_acceptor acceptor_type;
            // HACK:
            typedef int *arg_type;
            static constexpr uint16_t default_port=80;
            static stream_type *construct(arg_type) {
                return new stream_type;
            }
        };
        
        template<>
        struct stream_traits<ssl::tcp_stream> {
            typedef ssl::tcp_stream stream_type;
            typedef ssl::tcp_stream_acceptor acceptor_type;
            // HACK:
            typedef ssl::context *arg_type;
            static constexpr uint16_t default_port=443;
            static stream_type *construct(arg_type arg) {
                return new stream_type(*arg);
            }
        };
        
        template<typename Stream>
        struct connection {
            typedef stream_traits<Stream> traits_type;
            typedef typename traits_type::stream_type stream_type;
            typedef typename traits_type::arg_type arg_type;

            connection(const std::string &host,
                       timeout_type read_timeout,
                       timeout_type write_timeout,
                       arg_type arg)
            : host_(host)
            , read_timeout_(read_timeout)
            , write_timeout_(write_timeout)
            , stream_(traits_type::construct(arg))
            {}
            
            connection(connection &&other)=default;
            connection(const connection &)=delete;
            ~connection() {
                close();
            }
            
            void start_watchdog() {
                watchdog_timer_.reset(new watchdog_timer_t(asio::get_io_service()));
                watchdog_fiber_.reset(new fiber(fiber::attributes(fiber::attributes::stick_with_parent),
                                                &connection::watchdog_fiber,
                                                this));
            }
            
            void watchdog_fiber() {
                boost::system::error_code ignore_ec;
                while (is_open()) {
                    watchdog_timer_->async_wait(asio::yield[ignore_ec]);
                    // close the stream if timeout
                    auto dur=watchdog_timer_->expires_from_now();
                    std::chrono::seconds s=std::chrono::duration_cast<std::chrono::seconds>(dur);
                    if (s <= std::chrono::seconds(0)) {
                        stream().close();
                    }
                }
            }
            
            bool recv(request &req) {
                bool ret=false;
                if (bad()) return false;
                if(read_timeout_>NO_TIMEOUT) {
                    // Set read timeout
                    watchdog_timer_->expires_from_now(read_timeout_);
                }
                ret=req.read(stream());
                return ret;
            }
            
            bool send(response &resp) {
                bool ret=false;
                if (bad()) return false;
                if(write_timeout_>NO_TIMEOUT) {
                    // Set write timeout
                    watchdog_timer_->expires_from_now(write_timeout_);
                }
                ret=resp.write(stream());
                if (!resp.keep_alive()) {
                    stream().close();
                    return false;
                }
                return ret;
            }
            
            bool is_open() const { return stream_ && stream().is_open(); }
            
            void close() {
                if (stream_) {
                    stream_->close();
                    stream_.reset();
                }
                if (watchdog_timer_) {
                    watchdog_timer_->cancel();
                }
                if (watchdog_fiber_) {
                    watchdog_fiber_->join();
                    watchdog_fiber_.reset();
                }
                watchdog_timer_.reset();
            }
            
            stream_type &stream() { return *stream_; };
            const stream_type &stream() const { return *stream_; };

            bool bad() const {
                if(!stream_) return true;
                return !stream().is_open() || stream().eof() || stream().fail() || stream().bad();
            }
            
            bool good() const {
                return !bad();
            }
            
            const std::string &host_;
            timeout_type read_timeout_;
            timeout_type write_timeout_;
            
            std::unique_ptr<stream_type> stream_;
            std::unique_ptr<watchdog_timer_t> watchdog_timer_;
            std::unique_ptr<fiber> watchdog_fiber_;
        };
        
        template<typename Stream>
        struct server_engine {
            typedef stream_traits<Stream> traits_type;
            typedef typename traits_type::stream_type stream_type;
            typedef typename traits_type::acceptor_type acceptor_type;
            typedef typename traits_type::arg_type arg_type;
            typedef connection<Stream> connection_type;
            
            server_engine(arg_type arg,
                          const std::string &addr,
                          unsigned short port,
                          const std::string &host,
                          server::request_handler_type default_request_handler)
            : host_(host)
            , acceptor_(addr.c_str(), port)
            , default_request_handler_(std::move(default_request_handler))
            , arg_(arg)
            , active_connection_(0)
            {}

            server_engine(unsigned short port, const std::string &host)
            : host_(host)
            , acceptor_(port)
            {}
            
            void start() {
                watchdog_.reset(new fiber(fiber::attributes(fiber::attributes::stick_with_parent),
                                          &server_engine::watchdog,
                                          this));
                boost::system::error_code ec;
                // Loop until accept closed
                while (true) {
                    connection_type sc(host_, read_timeout_, write_timeout_, arg_);
                    ec=accept(sc);
                    if(ec) break;
                    sc.read_timeout_=read_timeout_;
                    sc.write_timeout_=write_timeout_;
                    fiber(&server_engine::servant, this, std::move(sc)).detach();
                }
                watchdog_->join();
            }
            
            void close() {
                exit_signal_.set_value();
                if(watchdog_)
                    watchdog_->join();
                // Wait until all connections are closed
                std::unique_lock<mutex> l(connection_counter_mtx_);
                while(active_connection_) {
                    connection_close_.wait(l);
                }
            }
            
            boost::system::error_code accept(connection_type &sc) {
                boost::system::error_code ec;
                acceptor_(sc.stream(), ec);
                if (!ec) {
                    active_connection_++;
                }
                return ec;
            }
            
            void watchdog() {
                exit_signal_.get_future().wait();
                acceptor_.close();
            }
            
            void servant(connection_type c) {
                if (read_timeout_>NO_TIMEOUT || write_timeout_>NO_TIMEOUT) {
                    c.start_watchdog();
                }
                request req;
                int count=0;
                while(c.recv(req)) {
                    response resp;
                    req.raw_stream_=&(c.stream());
                    resp.raw_stream_=&(c.stream());
                    // Set default attributes for response
                    resp.status_code(http_status_code::OK);
                    resp.version(req.version);
                    resp.keep_alive(req.keep_alive);
                    if(count>=max_keep_alive_) resp.keep_alive(false);
                    if(!default_request_handler_(req, resp)) {
                        break;
                    }
                    resp.body_stream().flush();
                    c.send(resp);
                    // Make sure we consumed all parts of the request
                    req.drop_body();
                    // Make sure all data are received and sent
                    c.stream().flush();
                    // Keepalive counter
                    count++;
                }
                c.close();
                
                active_connection_--;
                connection_close_.notify_one();
            }
            
            std::string host_;
            acceptor_type acceptor_;
            server::request_handler_type default_request_handler_;
            promise<void> exit_signal_;
            timeout_type read_timeout_=DEFAULT_TIMEOUT;
            timeout_type write_timeout_=DEFAULT_TIMEOUT;
            unsigned max_keep_alive_=DEFAULT_MAX_KEEP_ALIVE;
            arg_type arg_;
            
            std::unique_ptr<fiber> watchdog_;
            
            // connection uses vhost info in server
            // make sure server exists if there is living connection
            std::atomic<uint32_t> active_connection_;
            mutex connection_counter_mtx_;
            condition_variable connection_close_;
        };
    }   // End of namespace detail
    
    //////////////////////////////////////////////////////////////////////////////////////////
    // server_request
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void server_request::clear() {
        // Make sure there is no pending data in the last request
        drop_body();
        params.clear();
        common::request::clear();
    }
    
    bool server_request::accept_compressed() const {
        auto i=headers.find("Accept-Encoding");
        if (i==headers.end()) return false;
        // TODO: Kinda buggy
        return strcasestr(i->second.c_str(), "gzip")!=NULL;
    }
    
    bool server_request::read(std::istream &is) {
        clear();
        if (!common::request::read_header(is)) return false;
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
    
    const common::cookie_map &server_request::cookies() {
        if (!cookies_) {
            cookies_.reset(new common::cookie_map);
            common::parse_cookie(headers, *cookies_, false);
        }
        return *cookies_;
    }
    
    void server_request::drop_body() {
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
    // server_response
    //////////////////////////////////////////////////////////////////////////////////////////
    
    void server_response::clear() {
        common::response::clear();
        std::string e;
        if (!raw_body_stream_.vector().empty())
            raw_body_stream_.swap_vector(e);
    }
    
    const std::string &server_response::body() const {
        return raw_body_stream_.vector();
    }
    
    std::ostream &server_response::body_stream() {
        return raw_body_stream_;
    }
    
    server_response &server_response::header(const std::string &key, const std::string &value) {
        headers.insert({key, value});
        return *this;
    }
    
    server_response &server_response::cookie(const common::cookie &c) {
        headers.insert({"Set-Cookie", c.to_string()});
        return *this;
    }
    
    size_t server_response::content_length() const {
        return raw_body_stream_.vector().size();
    }
    
    server_response &server_response::content_type(const std::string &ct) {
        auto i=headers.find("content-type");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Content-Type", ct));
        } else {
            i->second.assign(ct);
        }
        return *this;
    }
    
    bool server_response::write_header(std::ostream &os) {
        std::string ka;
        if (keep_alive()) {
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
        if (!common::response::write_header(os)) return false;
        return !os.eof() && !os.fail() && !os.bad();
    }
    
    bool server_response::write(std::ostream &os) {
        // Set "content-length" header
        auto i=headers.find("content-length");
        if (i==headers.end()) {
            headers.insert(std::make_pair("Content-Length", boost::lexical_cast<std::string>(content_length())));
        } else {
            i->second.assign(boost::lexical_cast<std::string>(content_length()));
        }
        // Write headers
        if (!write_header(os)) return false;
        // Write body
        os.write(&(raw_body_stream_.vector()[0]), raw_body_stream_.vector().size());
        os.flush();
        return !os.eof() && !os.fail() && !os.bad();
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // server
    //////////////////////////////////////////////////////////////////////////////////////////
    
    typedef detail::server_engine<tcp_stream> server_engine;
    typedef detail::server_engine<ssl::tcp_stream> ssl_server_engine;
    
    static inline server_engine *get_engine(server::impl *impl) {
        return reinterpret_cast<server_engine *>(impl);
    }

    static inline ssl_server_engine *get_ssl_engine(server::impl *impl) {
        return reinterpret_cast<ssl_server_engine *>(impl);
    }
    
    template<typename Stream>
    static std::string get_default_host_name(uint16_t port) {
        std::string ret="127.0.0.1";
        if(detail::stream_traits<Stream>::default_port!=port) {
            ret+=':';
            ret+=boost::lexical_cast<std::string>(port);
        }
        return ret;
    }
    
    server::~server() {
        stop();
        if (ssl()) {
            delete get_ssl_engine(engine_);
        } else {
            delete get_engine(engine_);
        }
    }
    
    void server::init_engine() {
        if(ssl()) {
            engine_=reinterpret_cast<impl *>(new ssl_server_engine(s_.ctx_,
                                                                   s_.address_,
                                                                   s_.port_,
                                                                   get_default_host_name<ssl::tcp_stream>(s_.port_),
                                                                   std::move(s_.default_request_handler_)));
            get_ssl_engine(engine_)->read_timeout_=s_.read_timeout_;
            get_ssl_engine(engine_)->write_timeout_=s_.write_timeout_;
            get_ssl_engine(engine_)->max_keep_alive_=s_.max_keep_alive_;
        } else {
            engine_=reinterpret_cast<impl *>(new server_engine(0,
                                                               s_.address_,
                                                               s_.port_,
                                                               get_default_host_name<tcp_stream>(s_.port_),
                                                               std::move(s_.default_request_handler_)));
            get_engine(engine_)->read_timeout_=s_.read_timeout_;
            get_engine(engine_)->write_timeout_=s_.write_timeout_;
            get_engine(engine_)->max_keep_alive_=s_.max_keep_alive_;
        }
    }
    
    server &server::start() {
        init_engine();
        if (ssl()) {
            servant_.reset(new fiber(&ssl_server_engine::start, get_ssl_engine(engine_)));
        } else {
            servant_.reset(new fiber(&server_engine::start, get_engine(engine_)));
        }
        return *this;
    }
    
    void server::stop() {
        if (servant_) {
            if (ssl()) {
                get_ssl_engine(engine_)->close();
            } else {
                get_engine(engine_)->close();
            }
        }
    }
    
    void server::join() {
        if (servant_) {
            servant_->join();
            servant_.reset();
        }
    }
}}  // End of namespace fibio::http
