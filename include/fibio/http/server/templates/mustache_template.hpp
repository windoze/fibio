//
//  mustache_template.hpp
//  fibio
//
//  Created by Chen Xu on 14/12/1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_http_server_template_hpp
#define fibio_http_server_template_hpp

#include <fibio/http/server/templates/json.hpp>
#include <fibio/http/server/templates/mustache.hpp>
#include <fibio/http/server/routing.hpp>

namespace fibio { namespace http {
    template<typename Fn>
    inline server::request_handler_type mustache_(const std::string &tmpl, Fn &&fn) {
        static_assert(std::is_convertible<typename std::result_of<Fn(server::request &)>::type, json::wvalue>::value,
                      "Model functor must return json::wvalue object");
        struct view {
            bool operator()(server::request &req,
                            server::response &resp)
            {
                try {
                    resp.set_content_type("text/html");
                    json::wvalue ctx=model(req);
                    std::string s(t.render(ctx));
                    resp.body_stream().write(&(s[0]), s.size());
                    resp.body_stream().flush();
                    return true;
                } catch (std::exception &) {
                    resp.status_code=common::http_status_code::INTERNAL_SERVER_ERROR;
                }
                return false;
            }
            mustache::template_t t;
            Fn model;
        };
        return view{mustache::compile(tmpl), std::forward<Fn>(fn)};
    }
}}  // End of namespace fibio::http
#endif
