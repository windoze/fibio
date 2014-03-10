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
#include <fibio/stream/iostream.hpp>

namespace fibio { namespace fibers {
    namespace detail {
        struct fiberized_std_stream_guard {
            typedef stream::streambuf<io::posix::stream_descriptor> sbuf_t;
            typedef sbuf_t *sbuf_ptr_t;
            
            fiberized_std_stream_guard();
            ~fiberized_std_stream_guard();
            
            std::streambuf *old_cin_buf_;
            std::streambuf *old_cout_buf_;
            std::streambuf *old_cerr_buf_;
            sbuf_ptr_t cin_buf_;
            sbuf_ptr_t cout_buf_;
            sbuf_ptr_t cerr_buf_;
        };
        
        template<typename T>
        struct to_int_if_void {
            typedef T type;
            template<class = typename std::enable_if<std::is_move_assignable<T>::value>::type >
            inline static void assign(T &lhs, T&&rhs) {
                lhs=std::move(rhs);
            }
            template<class = typename std::enable_if<!std::is_move_assignable<T>::value>::type >
            inline static void assign(T &lhs, const T& rhs) {
                lhs=std::move(rhs);
            }
        };
        
        template<>
        struct to_int_if_void<void> {
            typedef int type;
            inline static void assign(type &lhs, ...) {
                lhs=0;
            }
        };
    }   // End of namespace fibio::fibers::detail
    
    template<typename Fn, typename ...Args,
        typename Ret=typename detail::to_int_if_void<typename std::result_of<Fn(Args...)>::type>::type
    >
    typename std::result_of<Fn(Args...)>::type fiberize(size_t nthr, Fn &&fn, Args&& ...args) {
        typedef typename std::result_of<Fn(Args...)>::type ActualRet;
        Ret ret;
        try
        {
            fibio::scheduler sched=fibio::scheduler::get_instance();
            sched.start(nthr);
            fibio::fiber f([&](){
                detail::fiberized_std_stream_guard fg;
                detail::to_int_if_void<ActualRet>::assign(ret, fn(args...));
                //ret=fn(args...);
                
            });
            sched.join();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        fibio::scheduler::reset_instance();
        return (ActualRet)(ret);
    }
}}  // End of namespace fibio::fibers

#endif
