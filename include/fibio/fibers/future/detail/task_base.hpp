//
//  task_base.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
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
    template<typename R, typename ...Args>
    struct task_base : public shared_state<R> {
        typedef boost::intrusive_ptr<task_base>  ptr_t;
        virtual ~task_base() {}
        virtual void run(Args&&... args) = 0;
    };
}}} // End of namespace fibio::fibers::detail

#endif
