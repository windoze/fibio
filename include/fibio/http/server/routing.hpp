//
//  routing.hpp
//  fibio-http
//
//  Created by Chen Xu on 14/10/12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_routing_hpp
#define fibio_http_server_routing_hpp

#include <list>
#include <functional>
#include <boost/optional.hpp>
#include <fibio/utility.hpp>
#include <fibio/http/common/string_pred.hpp>
#include <fibio/http/common/url_parser.hpp>
#include <fibio/http/server/server.hpp>

namespace fibio { namespace http {
    namespace detail {
        template<typename C, typename Pred>
        bool find_and_test(const C &c, typename C::key_type &k, Pred pred) {
            auto i=c.find(k);
            if(i!=c.end()) return pred(i->second());
            return false;
        }

        // Helper functions 
        template<typename Ret, class... Args>
        struct function_wrapper;
        
        // function_wrapper
        template<typename Ret, class... Args>
        struct function_wrapper<std::function<Ret(Args...)>> {
            typedef std::function<Ret(Args...)> function_type;
            typedef std::tuple<typename std::decay<Args>::type...> arg_list_type;
            typedef Ret result_type;
            static constexpr size_t arity=sizeof...(Args);
            function_wrapper(function_type &&f) : f_(std::forward<function_type>(f)) {}
            template<typename T, size_t N>
            T get(const server::request &req) const { return boost::lexical_cast<T>(req.params[N].second); }
            template <std::size_t... Indices>
            result_type call2(server::request &req, server::response &resp, utility::tuple_indices<Indices...>)
            { return utility::invoke(f_, get<typename std::tuple_element<Indices, arg_list_type>::type, Indices>(req)...); }
            bool call(server::request &req, server::response &resp) {
                if (arity!=req.params.size()) { throw server_error(http_status_code::BAD_REQUEST); }
                result_type r=call2(req,
                                    resp,
                                    typename utility::make_tuple_indices<std::tuple_size<std::tuple<Args...>>::value>::type());
                resp.body(r);
                return true;
            }
            function_type f_;
        };
        
        // function_wrapper for handlers require req and resp parameters
        template<class... Args>
        struct function_wrapper<std::function<bool(server::request &, server::response &, Args...)>> {
            typedef std::function<bool(server::request &, server::response &, Args...)> function_type;
            typedef std::tuple<typename std::decay<Args>::type...> arg_list_type;
            typedef void result_type;
            static constexpr size_t arity=sizeof...(Args);
            function_wrapper(function_type &&f) : f_(std::forward<function_type>(f)) {}
            template<typename T, size_t N>
            T get(const server::request &req) const { return boost::lexical_cast<T>(req.params[N].second); }
            template <std::size_t... Indices>
            bool call2(server::request &req, server::response &resp, utility::tuple_indices<Indices...>) {
                return utility::invoke(f_,
                                       req,
                                       resp,
                                       get<typename std::tuple_element<Indices, arg_list_type>::type, Indices>(req)...);
            }
            bool call(server::request &req, server::response &resp){
                if (arity!=req.params.size()) { throw server_error(http_status_code::BAD_REQUEST); }
                return call2(req,
                             resp,
                             typename utility::make_tuple_indices<std::tuple_size<std::tuple<Args...>>::value>::type());
            }
            function_type f_;
        };
        
        template<typename F>
        bool apply(server::request &req, server::response &resp, F f) {
            typedef detail::function_wrapper<utility::make_function_type<F>> wrapper;
            return wrapper(utility::make_function(std::forward<F>(f))).call(req, resp);
        }

        inline void continue_processor(server::request &req, server::response &resp)
        {
            // This function will only be called when request is "matched",
            // that means all header-based checks are passed.
            // We just send 100 response back immediately
            if(boost::iequals(req.header("Expect"), "100-continue")) {
                resp.raw_stream() << "HTTP/1.1 100 Continue\r\n\r\n";
                resp.raw_stream().flush();
            }
        }
    }
    
    typedef std::function<bool(server::request &)> match;
    typedef std::pair<match, server::request_handler> routing_rule;
    struct routing_table : std::list<routing_rule> {
        typedef std::list<routing_rule> base_type;
        using base_type::base_type;
        routing_table()=default;
        routing_table(const routing_table &)=default;
        routing_table(routing_table &&)=default;
        routing_table &operator=(const routing_table &)=default;
        routing_table &operator=(routing_table &&)=default;
        void push_back(const routing_rule &r) { base_type::push_back(r); }
        void push_back(routing_rule &&r) { base_type::push_back(std::move(r)); }
        void push_back(const routing_table &r) { insert(end(), r.begin(), r.end()); }
        void push_back(routing_table &&r) { splice(end(), r); }

        template<typename T>
        static void make_routing_table_impl(routing_table &table, T &&t)
        { table.push_back(std::move(t)); }
        
        template<typename T, typename... Args>
        static void make_routing_table_impl(routing_table &table, T &&t, Args&&... args) {
            table.push_back(std::move(t));
            make_routing_table_impl(table, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        static routing_table make(Args&&... args) {
            routing_table table;
            make_routing_table_impl(table, std::forward<Args>(args)...);
            return table;
        }
    };
    
    /**
     * Match functor operators
     */
    inline match operator&&(const match &lhs, const match &rhs)
    { return [=](server::request &req)->bool{ return lhs(req) && rhs(req); }; }
    
    inline match operator||(const match &lhs, const match &rhs)
    { return [=](server::request &req)->bool{ return lhs(req) || rhs(req); }; }
    
#ifndef _MSC_VER
    // Conflict with something in ppltasks.h?
    inline match operator!(const match &m)
    { return [=](server::request &req)->bool{ return !m(req); }; }
#endif
    inline match not_(const match &m)
    { return [=](server::request &req)->bool{ return !m(req); }; }

    /**
     * Match all
     */
    inline match match_all()
    { return [=](server::request &)->bool{ return true; }; }
    
    /**
     * Match nothing
     */
    inline match match_none()
    { return [=](server::request &)->bool{ return false; }; }

    /**
     * Match HTTP method
     */
    inline match method_is(http_method m)
    { return [=](server::request &req)->bool{ return req.method==m; }; }
    
    /**
     * Match HTTP version
     */
    inline match version_is(http_version v)
    { return [=](server::request &req)->bool{ return req.version==v; }; }
    
    /**
     * Check URL against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    match url_(Predicate pred)
    { return [=](server::request &req)->bool { return pred(req.url); }; }

    /**
     * Check specific header against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    inline match header_(const std::string &h, Predicate pred)
    { return [=](server::request &req)->bool { return detail::find_and_test(req.headers, h, pred); }; }
    
    /**
     * Check specific matched parameter against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    inline match param_(const std::string &p, Predicate pred)
    { return [=](server::request &req)->bool { return detail::find_and_test(req.params, p, pred); }; }
    
    // Match path pattern and extract parameters into match_info
    match path_(const std::string &tmpl);
    
    // Convenience
    inline match get_(const std::string &pattern)
    { return method_is(http_method::GET) && path_(pattern); }
    
    inline match post_(const std::string &pattern)
    { return method_is(http_method::POST) && path_(pattern); }
    
    inline match put_(const std::string &pattern)
    { return (method_is(http_method::PUT) || method_is(http_method::PATCH)) && path_(pattern); }

    inline match delete_(const std::string &pattern)
    { return (method_is(http_method::DELETE_) || method_is(http_method::PURGE)) && path_(pattern); }
    
    inline match basic_auth(const std::string &realm, std::function<bool(const std::string &, const std::string &)> chk) {
        struct matcher {
            matcher(const std::string &realm, std::function<bool(const std::string &, const std::string &)> chk)
            : chk_(chk)
            {
                hdr_.insert({"WWW-Authenticate", std::string("Basic realm=\"")+realm+"\""});
            }
            
            bool operator()(server::request &req) const {
                std::string auth_hdr=req.header("Authorization");
                if(!auth_hdr.empty()) {
                    if(auth_hdr.compare(0, 6, "Basic ")==0){
                        std::string auth=common::base64_decode(std::string(auth_hdr.begin()+6, auth_hdr.end()));
                        size_t colon=auth.find(':');
                        if(colon!=std::string::npos) {
                            std::string user(auth.begin(), auth.begin()+colon);
                            std::string pass(auth.begin()+colon+1, auth.end());
                            if(chk_(user, pass)) {
                                return true;
                            }
                        }
                    }
                }
                throw server_error(http_status_code::UNAUTHORIZED, "Authorization required", hdr_);
            }
            
            common::header_map hdr_;
            std::function<bool(const std::string &, const std::string &)> chk_;
        };
        return matcher(realm, chk);
    }
    
    /**
     * Stock response with specific status code, can be used with http server or routing table
     */
    inline server::request_handler stock_handler(http_status_code c)
    { return [=](server::request &, server::response &resp)->bool{ resp.status_code(c); return true; }; }
    
    inline server::request_handler handler_(server::request_handler &&func){
        return std::forward<server::request_handler>(func);
    }
    
    template<typename Fn>
    inline server::request_handler handler_(Fn func){
        return server::request_handler([func](server::request &req, server::response &resp) {
            return detail::apply(req, resp, func);
        });
    }
    
    inline server::request_handler post_proc_(server::request_handler func,
                                              std::function<void(server::response &)> post_proc) {
        return server::request_handler([func, post_proc](server::request &req, server::response &resp){
            bool ret=func(req, resp);
            post_proc(resp);
            return ret;
        });
    }

    inline server::request_handler post_proc_(server::request_handler func,
                                              std::function<void(server::request &, server::response &)> post_proc) {
        return [func, post_proc](server::request &req, server::response &resp){
            bool ret=func(req, resp);
            post_proc(req, resp);
            return ret;
        };
    }
    
    /**
     * Routing table
     */
    template<typename Fn>
    inline auto rule(match &&m, Fn &&func)
    -> typename std::enable_if<!std::is_constructible<server::request_handler, Fn>::value, routing_rule>::type
    { return routing_rule{std::move(m), handler_(std::forward<Fn>(func))}; }
    
    inline routing_rule rule(match &&m, server::request_handler &&h)
    { return routing_rule{std::forward<match>(m), std::forward<server::request_handler>(h)}; }
    
    template<typename Fn>
    inline routing_rule operator >> (match &&m, Fn &&func)
    { return rule(std::forward<match>(m), std::forward<Fn>(func)); }
    
    template<typename Fn>
    inline server::request_handler with_content_type(const std::string &ct, Fn &&func)
    { return post_proc_(handler_(func), std::function<void(server::response &)>([ct](server::response &resp){ resp.content_type(ct); })); }
    
    template<typename Fn>
    inline server::request_handler html_(Fn &&func)
    { return with_content_type("text/html", func); }
    
    template<typename Fn>
    inline server::request_handler xml_(Fn &&func)
    { return with_content_type("text/xml", func); }
    
    inline server::request_handler route(const routing_table &table,
                                         server::request_handler default_handler=stock_handler(http_status_code::NOT_FOUND))
    {
        return [=](server::request &req, server::response &resp)->bool {
            parse_url(req.url, req.parsed_url);
            for(auto &e : table) if(e.first(req)) {
                // All header-based checks are passed, server should send 100 response
                detail::continue_processor(req, resp);
                return e.second(req, resp);
            }
            return default_handler(req, resp);
        };
    }
    
    inline server::request_handler route(routing_table &&table,
                                         server::request_handler default_handler=stock_handler(http_status_code::NOT_FOUND))
    {
        return [=](server::request &req, server::response &resp)->bool {
            parse_url(req.url, req.parsed_url);
            for(auto &e : table) if(e.first(req)) {
                // All header-based checks are passed, server should send 100 response
                detail::continue_processor(req, resp);
                return e.second(req, resp);
            }
            return default_handler(req, resp);
        };
    }
    
    template<typename... Rule>
    inline server::request_handler route(Rule&&... r)
    { return route(routing_table::make(std::forward<Rule>(r)...)); }
}}  // End of namespace fibio::http

#endif
