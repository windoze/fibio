//
//  routing.cpp
//  fibio-http
//
//  Created by Chen Xu on 14/10/12.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#include <fibio/http/server/routing.hpp>
#include "url_parser.hpp"

namespace fibio { namespace http {
    match_type operator&&(const match_type &lhs, const match_type &rhs) {
        struct matcher {
            bool operator()(server::request &req) {
                return lhs_(req) && rhs_(req);
            }
            match_type lhs_;
            match_type rhs_;
        };
        
        return matcher{lhs, rhs};
    }
    
    match_type operator||(const match_type &lhs, const match_type &rhs) {
        struct matcher {
            bool operator()(server::request &req) {
                return lhs_(req) || rhs_(req);
            }
            match_type lhs_;
            match_type rhs_;
        };
        
        return matcher{lhs, rhs};
    }
    
    match_type operator!(const match_type &m) {
        struct matcher {
            bool operator()(server::request &req) { return !op_(req); }
            match_type op_;
        };
        
       return matcher{m};
    }
    
    server::request_handler_type route(const routing_table_type &table,
                                       server::request_handler_type default_handler)
    {
        struct handler {
            bool operator()(server::request &req,
                            server::response &resp,
                            server::connection &conn)
            {
                parse_url(req.url, req.parsed_url);
                for(auto &e : routing_table_) {
                    if(e.first(req)) {
                        return e.second(req, resp, conn);
                    }
                }
                return default_handler_(req, resp, conn);
            }
            
            routing_table_type routing_table_;
            server::request_handler_type default_handler_;
        };
        
        return handler{table, default_handler};
    }

    server::request_handler_type subroute(const routing_table_type &table,
                                          server::request_handler_type default_handler)
    {
        struct handler {
            bool operator()(server::request &req,
                            server::response &resp,
                            server::connection &conn)
            {
                parse_url(req.url, req.parsed_url);
                for(auto &e : routing_table_) {
                    if(e.first(req)) {
                        return e.second(req, resp, conn);
                    } else {
                        req.params.clear();
                    }
                }
                return default_handler_(req, resp, conn);
            }
            
            routing_table_type routing_table_;
            server::request_handler_type default_handler_;
        };
        
        return handler{table, default_handler};
    }
    
    match_type match_any() {
        struct matcher {
            bool operator()(server::request &) const
            { return true; }
        };
        return matcher();
    }
    
    const match_type any;

    match_type method_is(http_method m) {
        struct matcher {
            bool operator()(server::request &req) const {
                return req.method==method_;
            }
            http_method method_;
        };
        return matcher{m};
    }
    
    match_type version_is(http_version v) {
        struct matcher {
            bool operator()(server::request &req) const {
                return req.version==version_;
            }
            http_version version_;
        };
        return matcher{v};
    }
    
    match_type path_(const std::string &tmpl) {
        struct matcher {
            typedef std::list<std::string> components_type;
            typedef components_type::const_iterator component_iterator;
            
            bool operator()(server::request &req) {
                std::map<std::string, std::string> m;
                parse_url(req.url, req.parsed_url);
                component_iterator p=pattern.cbegin();
                for (auto &i : req.parsed_url.path_components) {
                    if (i.empty()) {
                        // Skip empty component
                        continue;
                    }
                    if (p==pattern.cend()) {
                        // End of pattern
                        return false;
                    } else if ((*p)[0]==':') {
                        // This pattern component is a parameter
                        m.insert({std::string(p->begin()+1, p->end()), i});
                    } else if ((*p)[0]=='*') {
                        if (p->length()==1) {
                            // Ignore anything remains if the wildcard doesn't have a name
                            return true;
                        }
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
                if (ret) {
                    std::move(m.begin(), m.end(), std::inserter(req.params, req.params.end()));
                }
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
    
    match_type GET(const std::string &pattern) {
        return method_is(http_method::GET) && path_(pattern);
    }
    
    match_type POST(const std::string &pattern) {
        return method_is(http_method::POST) && path_(pattern);
    }
    
    match_type PUT(const std::string &pattern) {
        return method_is(http_method::PUT) && path_(pattern);
    }
}}  // End of namespace fibio::http
