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
    
    inline match operator!(const match &m)
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
    inline match path_(const std::string &tmpl) {
        typedef std::list<std::string> components_type;
        typedef components_type::const_iterator component_iterator;
        struct matcher {
            bool operator()(server::request &req) {
                std::map<std::string, std::string> m;
                parse_url(req.url, req.parsed_url);
                component_iterator p=pattern.cbegin();
                for (auto &i : req.parsed_url.path_components) {
                    // Skip empty component
                    if (i.empty()) continue;
                    if (p==pattern.cend()) {
                        // End of pattern
                        return false;
                    } else if ((*p)[0]==':') {
                        // This pattern component is a parameter
                        m.insert({std::string(p->begin()+1, p->end()), i});
                    } else if ((*p)[0]=='*') {
                        // Ignore anything remains if the wildcard doesn't have a name
                        if (p->length()==1) return true;
                        std::string param_name(p->begin()+1, p->end());
                        //auto mi=m.find(param_name);
                        auto mi=std::find_if(m.begin(), m.end(), [&](const std::pair<std::string, std::string> &e){return e.first==param_name;});
                        if (mi==m.end()) {
                            // Not found
                            m.insert({param_name, i});
                        } else {
                            // Concat this component to existing parameter
                            mi->second.push_back('/');
                            mi->second.append(i);
                        }
                        // NOTE: Do not increment p
                        continue;
                    } else if (*p!=i) {
                        // Not match
                        return false;
                    }
                    ++p;
                }
                // Either pattern consumed or ended with a wildcard
                bool ret=(p==pattern.end() || (*p)[0]=='*');
                if (ret) std::move(m.begin(), m.end(), std::inserter(req.params, req.params.end()));
                return ret;
            }
            std::list<std::string> pattern;
            common::iequal eq;
        };
        matcher m;
        std::vector<std::string> c;
        common::parse_path_components(tmpl, m.pattern);
        return std::move(m);
    }
    
    // Convenience
    inline match get_(const std::string &pattern)
    { return method_is(http_method::GET) && path_(pattern); }
    
    inline match post_(const std::string &pattern)
    { return method_is(http_method::POST) && path_(pattern); }
    
    inline match put_(const std::string &pattern)
    { return (method_is(http_method::PUT) || method_is(http_method::PATCH)) && path_(pattern); }

    inline match delete_(const std::string &pattern)
    { return (method_is(http_method::DELETE) || method_is(http_method::PURGE)) && path_(pattern); }
    
    /**
     * Stock response with specific status code, can be used with http server or routing table
     */
    server::request_handler stock_handler(http_status_code c)
    { return [=](server::request &, server::response &resp)->bool{ resp.status_code(c); return true; }; }
    
    template<typename Fn>
    inline server::request_handler handler_(Fn func){
        return [func](server::request &req, server::response &resp)
        { return detail::apply(req, resp, func); };
    }
    
    template<typename Fn>
    inline server::request_handler handler_(Fn func, std::function<void(server::response &)> post_proc) {
        return [func, post_proc](server::request &req, server::response &resp){
            bool ret=detail::apply(req, resp, func);
            post_proc(resp);
            return ret;
        };
    }
    
    template<typename Fn>
    inline server::request_handler handler_(Fn func, std::function<void(server::request &, server::response &)> post_proc) {
        return [func, post_proc](server::request &req, server::response &resp){
            bool ret=detail::apply(req, resp, func);
            post_proc(req, resp);
            return ret;
        };
    }
    
    /**
     * Routing table
     */
    template<typename Fn,
        typename std::enable_if<!std::is_constructible<server::request_handler, Fn>::value>::type* = nullptr
    >
    inline routing_rule rule(match &&m, Fn &&func)
    { return routing_rule{std::move(m), handler_(std::forward<Fn>(func))}; }
    
    inline routing_rule rule(match &&m, server::request_handler &&h)
    { return routing_rule{std::forward<match>(m), std::forward<server::request_handler>(h)}; }
    
    template<typename Fn>
    inline routing_rule operator >> (match &&m, Fn &&func)
    { return rule(std::forward<match>(m), std::forward<Fn>(func)); }
    
    template<typename Fn>
    inline server::request_handler with_content_type(const std::string &ct, Fn &&func)
    { return handler_(func, [ct](server::response &resp){ resp.content_type(ct); }); }
    
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
            for(auto &e : table) if(e.first(req)) return e.second(req, resp);
            return default_handler(req, resp);
        };
    }
    
    inline server::request_handler route(routing_table &&table,
                                         server::request_handler default_handler=stock_handler(http_status_code::NOT_FOUND))
    {
        return [=](server::request &req, server::response &resp)->bool {
            parse_url(req.url, req.parsed_url);
            for(auto &e : table) if(e.first(req)) return e.second(req, resp);
            return default_handler(req, resp);
        };
    }
    
    template<typename... Rule>
    inline server::request_handler route(Rule&&... r)
    { return route(routing_table::make(std::forward<Rule>(r)...)); }
}}  // End of namespace fibio::http

#endif
