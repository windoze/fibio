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
    }
    
    typedef std::function<bool(server::request &)> match_type;
    typedef std::pair<match_type, server::request_handler_type> routing_rule_type;
    typedef std::list<routing_rule_type> routing_table_type;
    
    /**
     * Match functor operators
     */
    inline match_type operator&&(const match_type &lhs, const match_type &rhs)
    { return [=](server::request &req)->bool{ return lhs(req) && rhs(req); }; }
    
    inline match_type operator||(const match_type &lhs, const match_type &rhs)
    { return [=](server::request &req)->bool{ return lhs(req) || rhs(req); }; }
    
    inline match_type operator!(const match_type &m)
    { return [=](server::request &req)->bool{ return !m(req); }; }
    

    /**
     * Match all
     */
    inline match_type match_all()
    { return [=](server::request &)->bool{ return true; }; }
    
    /**
     * Match nothing
     */
    inline match_type match_none()
    { return [=](server::request &)->bool{ return false; }; }

    /**
     * Match HTTP method
     */
    inline match_type method_is(http_method m)
    { return [=](server::request &req)->bool{ return req.method==m; }; }
    
    /**
     * Match HTTP version
     */
    inline match_type version_is(http_version v)
    { return [=](server::request &req)->bool{ return req.version==v; }; }
    
    /**
     * Check URL against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    match_type url_(Predicate pred)
    { return [=](server::request &req)->bool { return pred(req.url); }; }

    /**
     * Check specific header against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    inline match_type header_(const std::string &h, Predicate pred)
    { return [=](server::request &req)->bool { return detail::find_and_test(req.headers, h, pred); }; }
    
    /**
     * Check specific matched parameter against pred
     * !see http/common/string_pred.hpp
     */
    template<typename Predicate>
    inline match_type param_(const std::string &p, Predicate pred)
    { return [=](server::request &req)->bool { return detail::find_and_test(req.params, p, pred); }; }
    
    // Match path pattern and extract parameters into match_info
    inline match_type path_(const std::string &tmpl) {
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
                        auto mi=m.find(param_name);
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
    inline match_type get_(const std::string &pattern)
    { return method_is(http_method::GET) && path_(pattern); }
    
    inline match_type post_(const std::string &pattern)
    { return method_is(http_method::POST) && path_(pattern); }
    
    inline match_type put_(const std::string &pattern)
    { return method_is(http_method::PUT) && path_(pattern); }

    /**
     * Stock response with specific status code, can be used with http server or routing table
     */
    server::request_handler_type stock_handler(http_status_code c)
    { return [=](server::request &, server::response &resp)->bool{ resp.status_code=c; return true; }; }
    
    /**
     * Routing table
     */
    inline routing_rule_type operator >> (match_type &&m, server::request_handler_type &&h)
    { return routing_rule_type{std::move(m), std::move(h)}; }
    
    inline server::request_handler_type route(const routing_table_type &table,
                                              server::request_handler_type default_handler=stock_handler(http_status_code::NOT_FOUND))
    {
        return [=](server::request &req, server::response &resp)->bool {
            parse_url(req.url, req.parsed_url);
            for(auto &e : table) if(e.first(req)) return e.second(req, resp);
            return default_handler(req, resp);
        };
    }
    
    template<typename... Rule>
    inline server::request_handler_type route(const Rule&... r)
    { return route(routing_table_type(routing_table_type{r...})); }
}}  // End of namespace fibio::http

#endif
