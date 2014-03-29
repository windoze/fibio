//
//  async.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_future_async_hpp
#define fibio_fibers_future_async_hpp

#include <memory>
#include <fibio/fibers/future/future.hpp>
#include <fibio/fibers/future/packaged_task.hpp>

namespace fibio { namespace fibers {
    template< typename F >
    future< typename std::result_of< F() >::type >
    async( F && f)
    {
        typedef typename std::result_of< F() >::type R;
        packaged_task< R() > pt( std::forward< F >( f) );
        future< R > fi( pt.get_future() );
        fiber( std::move( pt) ).detach();
        return std::move( fi);
    }
}}

namespace fibio {
    using fibers::async;
}

#endif
