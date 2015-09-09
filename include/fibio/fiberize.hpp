//
//  fiberize.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fiberize_hpp
#define fibio_fiberize_hpp

#include <type_traits>
#include <memory>
#include <iostream>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/windows/stream_handle.hpp>
#include <fibio/fibers/fiber.hpp>
#include <fibio/stream/streambuf.hpp>

/// Define this to stop using fiberized std stream
//#define FIBIO_DONT_FIBERIZE_STD_STREAM

/// Define this to stop using default main implementation
//#define FIBIO_DONT_USE_DEFAULT_MAIN

// Doesn't work under Windows
#if defined(_WIN32)
#define FIBIO_DONT_FIBERIZE_STD_STREAM
#endif

namespace fibio {
namespace fibers {
namespace detail {

#if !defined(FIBIO_DONT_FIBERIZE_STD_STREAM)

template <typename Stream,
          typename Desc,
          typename CharT = char,
          typename Traits = std::char_traits<CharT>>
struct guard
{
    typedef typename Desc::native_handle_type native_handle_type;
    typedef stream::streambuf<Desc> sbuf_t;
    typedef std::unique_ptr<sbuf_t> sbuf_ptr_t;

    guard(Stream& s, native_handle_type h, bool unbuffered = false)
    : new_buf_(new sbuf_t(this_fiber::detail::get_io_service())), stream_(s)
    {
        old_buf_ = stream_.rdbuf(new_buf_.get());
        new_buf_->assign(h);
        // Don't sync with stdio, it doesn't work anyway
        stream_.sync_with_stdio(false);
        // Set to unbuffered if needed
        if (unbuffered) stream_.rdbuf()->pubsetbuf(0, 0);
    }

    ~guard()
    {
        flush(stream_);
#if BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
        new_buf_->release();
#endif
        stream_.rdbuf(old_buf_);
    }

    // HACK: only ostream can be flushed
    void flush(std::basic_ostream<CharT, Traits>& s) { s.flush(); }

    void flush(std::basic_istream<CharT, Traits>& s) {}

    sbuf_ptr_t new_buf_;
    Stream& stream_;
    std::streambuf* old_buf_ = 0;
};

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
// POSIX platform
typedef boost::asio::posix::stream_descriptor std_handle_type;
typedef std_handle_type::native_handle_type native_handle_type;

native_handle_type std_in_handle()
{
    return 0;
}

native_handle_type std_out_handle()
{
    return 1;
}

native_handle_type std_err_handle()
{
    return 2;
}

#elif defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
// Windows platform
typedef boost::asio::windows::stream_handle std_handle_type;
typedef std_handle_type::native_handle_type native_handle_type;
native_handle_type std_in_handle()
{
    return ::GetStdHandle(STD_INPUT_HANDLE);
}
native_handle_type std_out_handle()
{
    return ::GetStdHandle(STD_OUTPUT_HANDLE);
}
native_handle_type std_err_handle()
{
    return ::GetStdHandle(STD_ERROR_HANDLE);
}
#else
#error("Unsupported platform")
#endif

struct std_stream_guard
{
    guard<std::istream, std_handle_type> cin_guard_{std::cin, std_in_handle()};
    guard<std::ostream, std_handle_type> cout_guard_{std::cout, std_out_handle()};
    guard<std::ostream, std_handle_type> cerr_guard_{std::cerr, std_err_handle(), true};
};

#else // !defined(FIBIO_DONT_FIBERIZE_STD_STREAM), No std stream guard, do nothing
struct std_stream_guard
{
};
#endif // !defined(FIBIO_DONT_FIBERIZE_STD_STREAM)

// HACK: C++ forbids variable with `void` type as it is always incomplete
template <typename T>
struct non_void
{
    typedef T type;

    template <typename Fn, typename... Args>
    static void run(type& t, Fn&& fn, Args&&... args)
    {
        t = std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
};

template <>
struct non_void<void>
{
    typedef int type;

    template <typename Fn, typename... Args>
    static void run(type& t, Fn&& fn, Args&&... args)
    {
        t = 0;
        std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
};

} // End of namespace detail

/**
 * Start the scheduler and create the first fiber in it
 *
 * @param sched the scheduler to run the fiber
 * @param fn the entry point of the fiber
 * @param args the arguments for the fiber
 */
// NOTE: I cannot make this specialization work in VC2015, gave up and changed the name...
template <typename Fn, typename... Args>
auto fiberize_with_sched(fibio::scheduler&& sched, Fn&& fn, Args&&... args) ->
    typename std::result_of<Fn(Args...)>::type
{
    typedef typename std::result_of<Fn(Args...)>::type result_type;
    typedef detail::non_void<result_type> non_void_result;
    typename non_void_result::type ret{};
    try {
        sched.start();
        fibio::fiber f(sched,
                       [&]() {
                           detail::std_stream_guard guard;
                           // Disable unused variable warning
                           (void)guard;
                           non_void_result::run(
                               ret, std::forward<Fn>(fn), std::forward<Args>(args)...);
                       });
        sched.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    // No-op if `result_type` is `void`
    return result_type(ret);
}

/**
 * Start the default scheduler and create the first fiber in it
 *
 * @param fn the entry point of the fiber
 * @param args the arguments for the fiber
 */
template <typename Fn, typename... Args>
auto fiberize(Fn&& fn, Args&&... args) -> typename std::result_of<Fn(Args...)>::type
{
    return fiberize_with_sched(
        fibio::scheduler::get_instance(), std::forward<Fn>(fn), std::forward<Args>(args)...);
}

} // End of namespace fibers

using fibers::fiberize_with_sched;
using fibers::fiberize;

} // End of namespace fibio

#if !defined(FIBIO_DONT_USE_DEFAULT_MAIN)
namespace fibio {
// Forward declaration of real entry point
int main(int argc, char* argv[]);
} // End of namespace fibio

int main(int argc, char* argv[])
{
    return fibio::fiberize(fibio::main, argc, argv);
}

#endif // !defined(FIBIO_DONT_USE_DEFAULT_MAIN)

#endif
