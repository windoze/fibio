//
//  shared_state_object.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef fibio_fibers_future_detail_shared_state_object_hpp
#define fibio_fibers_future_detail_shared_state_object_hpp

#include <boost/config.hpp>
#include <fibio/fibers/future/detail/shared_state.hpp>

namespace fibio {
namespace fibers {
namespace detail {

template <typename R, typename Allocator>
class shared_state_object : public shared_state<R>
{
public:
    typedef
        typename Allocator::template rebind<shared_state_object<R, Allocator>>::other allocator_t;

    shared_state_object(allocator_t const& alloc) : shared_state<R>(), alloc_(alloc) {}

protected:
    void deallocate_future() { destroy_(alloc_, this); }

private:
    allocator_t alloc_;

    static void destroy_(allocator_t& alloc, shared_state_object* p)
    {
        alloc.destroy(p);
        alloc.deallocate(p, 1);
    }
};

} // End of namespace detail
} // End of namespace fibers
} // End of namespace fibio

#endif
