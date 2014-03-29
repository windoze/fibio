//
//  task_base.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibio_fibers_future_detail_task_base_hpp
#define fibio_fibio_fibers_future_detail_task_base_hpp

#include <cstddef>

#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/future/detail/shared_state.hpp>

namespace fibio { namespace fibers { namespace detail {
    template< typename R >
    struct task_base : public shared_state< R >
    {
        typedef boost::intrusive_ptr< task_base >  ptr_t;
        
        virtual ~task_base() {}
        
        virtual void run() = 0;
    };
}}} // End of namespace fibio::fibers::detail

#endif
