//
//  use_future.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-31.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_asio_use_future_hpp
#define fibio_asio_use_future_hpp

#include <memory>

#include <boost/config.hpp>
#include <boost/asio/detail/config.hpp>

namespace fibio {
namespace fibers {
namespace asio {

    namespace detail {
        /**
         * HACK: Dummy allocator for void, as std::allocator<void> doesn't have construct and destroy
         */
        struct dummy_void_allocator : std::allocator<void> {
            template<typename T>
            void construct(void *, T) const {}
            void destroy(void *) const {}
        };
    }

/**
 * class use_future_t
 *
 * Instance of the class can be used as completion handler for Boost.ASIO async_* call,
 * makes calls return a fibio::future.
 *
 * @see fibio::future
 */
template <typename Allocator = detail::dummy_void_allocator>
class use_future_t
{
public:
    typedef Allocator allocator_type;

    /// Construct using default-constructed allocator.
    constexpr use_future_t() {}

    /// Construct using specified allocator.
    explicit use_future_t(Allocator const& allocator) : allocator_(allocator) {}

    /// Specify an alternate allocator.
    template <typename OtherAllocator>
    use_future_t<OtherAllocator> operator[](OtherAllocator const& allocator) const
    {
        return use_future_t<OtherAllocator>(allocator);
    }

    /// Obtain allocator.
    allocator_type get_allocator() const { return allocator_; }

private:
	const Allocator allocator_{};
};

/// The predefined instance of use_future_t can be used directly.
#ifdef _MSC_VER
// Visual Studio still has bug about constexpr
__declspec(selectany) use_future_t<> use_future;
#else
constexpr use_future_t<> use_future;
#endif

} // End of namespace asio
} // End of namespace fibers

namespace asio {

using fibers::asio::use_future;

} // End of namespace asio
} // End of namespace fibio

#include <fibio/fibers/asio/detail/use_future.hpp>

#endif
