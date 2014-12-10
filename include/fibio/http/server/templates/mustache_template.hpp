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
    namespace detail {
        template<typename Ret, typename ...Args>
        struct mustache_controller;
        
        template<typename Ret, typename ...Args>
        struct mustache_controller<std::function<Ret(Args...)>> {
            typedef std::function<Ret(Args...)> model_type;
            mustache_controller(const std::string &tmpl, model_type &&func)
            : view_(mustache::compile(tmpl))
            , model_(std::forward<model_type>(func))
            {
                static_assert(std::is_constructible<json::wvalue, Ret>::value,
                              "Return value of model function must be compatible with json::wvalue");
            }
            
            std::string operator()(Args&&... args) {
                json::wvalue ctx(model_(std::forward<Args>(args)...));
                return view_.render(ctx);
            }
            
            mustache::template_t view_;
            model_type model_;
        };
    }
    
    template<typename Fn>
    server::request_handler mustache_(const std::string &tmpl,
                                           Fn &&fn,
                                           const std::string content_type="text/html")
    {
        typedef typename utility::make_function_type<Fn> model_type;
        detail::mustache_controller<model_type> controller(tmpl, model_type(std::forward<Fn>(fn)));
        return with_content_type(content_type, controller);
    }
}}  // End of namespace fibio::http
#endif
