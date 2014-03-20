//
//  barrier.h
//  fibio
//
//  Created by Chen Xu on 14-3-20. Copied and modified from Boost.Thread
//
// Copyright (C) 2002-2003
// David Moore, William E. Kempf
// Copyright (C) 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef fibio_barrier_h
#define fibio_barrier_h

#include <type_traits>
#include <fibio/fibers/mutex.hpp>
#include <fibio/fibers/condition_variable.hpp>

namespace fibio { namespace fibers {
    namespace detail {
        typedef std::function<void()> void_completion_function;
        typedef std::function<size_t()> size_completion_function;
        struct default_barrier_reseter {
            unsigned int size_;
            default_barrier_reseter(unsigned int size) :
            size_(size) {}
            unsigned int operator()() {
                return size_;
            }
        };
        
        struct void_functor_barrier_reseter {
            unsigned int size_;
            void_completion_function fct_;
            template <typename F>
            void_functor_barrier_reseter(unsigned int size, F &&funct)
            : size_(size)
            , fct_(std::move(funct))
            {}

            unsigned int operator()() {
                fct_();
                return size_;
            }
        };
        
        struct void_fct_ptr_barrier_reseter {
            unsigned int size_;
            void(*fct_)();
            void_fct_ptr_barrier_reseter(unsigned int size, void(*funct)())
            : size_(size)
            , fct_(funct)
            {}
            unsigned int operator()() {
                fct_();
                return size_;
            }
        };
    }   // End of namespace fibio::fibers::detail

    class barrier {
        static inline unsigned int check_counter(unsigned int count) {
            if (count == 0)
                throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                        "barrier constructor: count cannot be zero.");
            return count;
        }
        
        struct dummy
        {};
        
    public:
        barrier(const barrier &)=delete;
        
        explicit barrier(unsigned int count)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(detail::default_barrier_reseter(count))
        {}
        
        template <typename F>
        barrier(unsigned int count,
                F&& funct,
                typename std::enable_if<std::is_void<typename std::result_of<F>::type>::value, dummy*>::type=0)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(detail::void_functor_barrier_reseter(count, std::move(funct)))
        {}
        
        template <typename F>
        barrier(unsigned int count,
                F&& funct,
                typename std::enable_if<std::is_same<typename std::result_of<F>::type, unsigned int>::value, dummy*>::type=0)
        : m_count(check_counter(count))
        , m_generation(0)
        , fct_(std::move(funct))
        {}
        
        barrier(unsigned int count, void(*funct)()) :
        m_count(check_counter(count)), m_generation(0),
        fct_(funct
             ? detail::size_completion_function(detail::void_fct_ptr_barrier_reseter(count, funct))
             : detail::size_completion_function(detail::default_barrier_reseter(count)))
        {}
        barrier(unsigned int count, unsigned int(*funct)()) :
        m_count(check_counter(count)), m_generation(0),
        fct_(funct
             ? detail::size_completion_function(funct)
             : detail::size_completion_function(detail::default_barrier_reseter(count)))
        {}
        
        bool wait() {
            unique_lock < mutex > lock(m_mutex);
            unsigned int gen = m_generation;
            
            if (--m_count == 0) {
                m_generation++;
                m_count = static_cast<unsigned int>(fct_());
                assert(m_count != 0);
                m_cond.notify_all();
                return true;
            }
            
            while (gen == m_generation)
                m_cond.wait(lock);
            return false;
        }
        
        void count_down_and_wait() {
            wait();
        }
        
    private:
        mutex m_mutex;
        condition_variable m_cond;
        unsigned int m_count;
        unsigned int m_generation;
        detail::size_completion_function fct_;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    using fibers::barrier;
}

#endif
