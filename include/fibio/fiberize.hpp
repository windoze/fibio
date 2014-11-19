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
#include <fibio/fibers/fiber.hpp>
#include <fibio/stream/streambuf.hpp>

namespace fibio { namespace fibers {
    namespace detail {
        struct fiberized_std_stream_guard {
            typedef stream::fiberized_streambuf<boost::asio::posix::stream_descriptor> sbuf_t;
            typedef std::unique_ptr<sbuf_t> sbuf_ptr_t;
            
            inline fiberized_std_stream_guard(boost::asio::io_service &iosvc=fibio::asio::get_io_service())
            : old_cin_buf_(0)
            , old_cout_buf_(0)
            , old_cerr_buf_(0)
            , cin_buf_(new sbuf_t())
            , cout_buf_(new sbuf_t())
            , cerr_buf_(new sbuf_t())
            {
                old_cin_buf_=std::cin.rdbuf(cin_buf_.get());
                old_cout_buf_=std::cout.rdbuf(cout_buf_.get());
                old_cerr_buf_=std::cerr.rdbuf(cerr_buf_.get());
                cin_buf_->assign(0);
                cout_buf_->assign(1);
                cerr_buf_->assign(2);
                // Set cerr to unbuffered
                std::cerr.rdbuf()->pubsetbuf(0, 0);
            }

            inline ~fiberized_std_stream_guard()
            {
                std::cout.flush();
                std::cerr.flush();
                cin_buf_->release();
                cout_buf_->release();
                cerr_buf_->release();
                std::cin.rdbuf(old_cin_buf_);
                std::cout.rdbuf(old_cout_buf_);
                std::cerr.rdbuf(old_cerr_buf_);
            }
            
            std::streambuf *old_cin_buf_;
            std::streambuf *old_cout_buf_;
            std::streambuf *old_cerr_buf_;
            sbuf_ptr_t cin_buf_;
            sbuf_ptr_t cout_buf_;
            sbuf_ptr_t cerr_buf_;
        };
    }   // End of namespace fibio::fibers::detail
    
    template<typename Fn, typename ...Args>
    auto fiberize(Fn &&fn, Args&& ...args) -> typename std::result_of<Fn(Args...)>::type {
        typedef typename std::result_of<Fn(Args...)>::type result_type;
        result_type ret;
        try {
            fibio::scheduler sched;
            sched.start();
            fibio::fiber f(sched, [&](){
                detail::fiberized_std_stream_guard guard;
                ret=fn(std::forward<Args>(args)...);
            });
            sched.join();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        return ret;
    }
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::fiberize;
#ifndef FIBIO_DONT_USE_DEFAULT_MAIN
    // Forward declaration of real entry point
    int main(int argc, char *argv[]);
#endif
}

#ifndef FIBIO_DONT_USE_DEFAULT_MAIN
int main(int argc, char *argv[]) {
    return fibio::fiberize(fibio::main, argc, argv);
}
#endif

#endif
