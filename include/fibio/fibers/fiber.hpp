//
//  fiber.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-1.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef __fibio__fiber__
#define __fibio__fiber__

#include <memory>
#include <functional>
#include <chrono>
#include <utility>
#include <type_traits>
#include <boost/asio/io_service.hpp>
#include <fibio/fibers/detail/forward.hpp>
#include <fibio/fibers/detail/fiber_data.hpp>

namespace fibio { namespace fibers {
    struct scheduler {
        scheduler();
        scheduler(std::shared_ptr<detail::scheduler_object>);
        
        boost::asio::io_service &get_io_service();
        
        void start(size_t nthr=1);
        void join();
        
        void add_worker_thread(size_t nthr=1);
        
        static scheduler get_instance();
        static void reset_instance();
        
        std::shared_ptr<detail::scheduler_object> impl_;
    };
    
    struct fiber {
        typedef uintptr_t id;
        
        struct attributes {
            enum scheduling_policy {
                // scheduled freely in this scheduler
                normal,
                // always runs in the same thread with parent
                stick_with_parent,
            };
            scheduling_policy policy;
            
            constexpr attributes(scheduling_policy p) : policy(p) {}
        };
        
        template <class F>
        explicit fiber(F &&f)
        : data_(detail::make_fiber_data(detail::decay_copy(std::forward<F>(f))))
        {
            start();
        }
        
        template <class F>
        fiber(attributes attrs, F &&f)
        : data_(detail::make_fiber_data(detail::decay_copy(std::forward<F>(f))))
        {
            start(attrs);
        }
        
        template <class F, class Arg, class ...Args>
        fiber(F&& f, Arg&& arg, Args&&... args)
        : data_(detail::make_fiber_data(detail::decay_copy(std::forward<F>(f)),
                                        detail::decay_copy(std::forward<Arg>(arg)),
                                        detail::decay_copy(std::forward<Args>(args))...)
                )
        
        {
            start();
        }
        
        template <class F, class Arg, class ...Args>
        fiber(attributes attrs, F&& f, Arg&& arg, Args&&... args)
        : data_(detail::make_fiber_data(detail::decay_copy(std::forward<F>(f)),
                                        detail::decay_copy(std::forward<Arg>(arg)),
                                        detail::decay_copy(std::forward<Args>(args))...)
                )
        {
            start(attrs);
        }
        
        explicit fiber(fiber &&other) noexcept;
        explicit fiber(scheduler &s, detail::fiber_data_ptr data);
        fiber() = default;
        fiber(const fiber&) = delete;
        
        fiber& operator=(fiber &&other) noexcept;
        
        void start();
        void start(attributes);
 
        bool joinable() const noexcept;
        id get_id() const noexcept;
        void join(bool propagate_exception=false);
        void detach();
        void swap(fiber &other) noexcept(true);
        
        static unsigned hardware_concurrency() noexcept;

        void set_name(const std::string &s);
        std::string get_name();
        
    //private:
        detail::fiber_data_ptr data_;
        std::shared_ptr<detail::fiber_object> impl_;
    };
    
    constexpr fiber::id not_a_fiber=0;
    
    namespace this_fiber {
        namespace detail {
            void sleep_usec(uint64_t usec);
            boost::asio::io_service &get_io_service();
        }

        void yield();
        
        fiber::id get_id();
        
        bool is_a_fiber() noexcept(true);
        
        template< class Rep, class Period >
        void sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration ) {
            detail::sleep_usec(std::chrono::duration_cast<std::chrono::microseconds>(sleep_duration).count());
        }
        
        template< class Clock, class Duration >
        void sleep_until( const std::chrono::time_point<Clock,Duration>& sleep_time ) {
            detail::sleep_usec(std::chrono::duration_cast<std::chrono::microseconds>(sleep_time - std::chrono::steady_clock::now()).count());
        }
    }   // End of namespace this_fiber
}}   // End of namespace fibio::fibers

namespace std {
    inline void swap(fibio::fibers::fiber &lhs, fibio::fibers::fiber &rhs) noexcept(true) {
        lhs.swap(rhs);
    }
}   // End of namespace std

namespace fibio {
    using fibers::scheduler;
    using fibers::fiber;
    namespace this_fiber {
        using fibers::this_fiber::yield;
        using fibers::this_fiber::get_id;
        using fibers::this_fiber::is_a_fiber;
        using fibers::this_fiber::sleep_for;
        using fibers::this_fiber::sleep_until;
    }
    namespace asio {
        using fibers::this_fiber::detail::get_io_service;
    }
}   // End of namespace fibio

#endif /* defined(__fibio__fiber__) */
