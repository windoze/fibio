//
//  packaged_task.hpp
//  fibio
//
//  Base on Boost.Fiber at https://github.com/olk/boost-fiber
//
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef fibio_fibers_future_packaged_task_hpp
#define fibio_fibers_future_packaged_task_hpp

#include <memory>

#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/thread/detail/memory.hpp> // boost::allocator_arg_t
#include <boost/throw_exception.hpp>
#include <boost/type_traits/decay.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility.hpp>

#include <fibio/fibers/exceptions.hpp>
#include <fibio/fibers/future/detail/task_base.hpp>
#include <fibio/fibers/future/detail/task_object.hpp>
#include <fibio/fibers/future/future.hpp>

namespace fibio { namespace fibers {
    template<typename>
    class packaged_task;
    
    template<typename R, typename ...Args>
    class packaged_task<R(Args...)> {
        typedef packaged_task<R(Args...)> this_type;
        typedef typename detail::task_base<R, Args...>::ptr_t ptr_t;
        
        bool obtained_=false;
        ptr_t task_;
        
        packaged_task(const packaged_task &)=delete;
        packaged_task& operator=( const packaged_task& ) = delete;
    public:
        packaged_task() noexcept
        : obtained_(false)
        , task_()
        {}
        
        ~packaged_task() { if(task_) { task_->owner_destroyed(); } }
        
        template<typename Fn,
            class = typename std::enable_if<!std::is_same<
                typename std::decay<Fn>::type,
                packaged_task
            >::value>::type
        >
        explicit packaged_task(Fn &&fn) {
            typedef detail::task_object<Fn, std::allocator<this_type>, R, Args...> object_t;
            std::allocator<packaged_task<R(Args...)>> alloc;
            typename object_t::allocator_t a(alloc);
            // placement new
            task_ = ptr_t(::new(a.allocate(1)) object_t(std::forward<Fn>(fn), a));
        }

        template<typename Fn, typename Allocator>
        explicit packaged_task(std::allocator_arg_t, const Allocator& alloc, Fn &&fn)
        {
            typedef detail::task_object<Fn, std::allocator<this_type>, R, Args...> object_t;
            typename object_t::allocator_t a(alloc);
            // placement new
            task_ = ptr_t(::new(a.allocate(1)) object_t(std::forward<Fn>(fn), a));
        }
        
        packaged_task(packaged_task&& other) noexcept
        : obtained_(false)
        , task_()
        { swap( other); }

        packaged_task &operator=(packaged_task&& other) noexcept
        {
            packaged_task tmp(std::move(other));
            swap(tmp);
            return *this;
        }
        
        void swap(packaged_task & other) noexcept
        {
            std::swap(obtained_, other.obtained_);
            task_.swap(other.task_);
        }

        bool valid() const noexcept { return task_.get()!=nullptr; }
        
        future<R> get_future()
        {
            if (obtained_) { BOOST_THROW_EXCEPTION(future_already_retrieved()); }
            if (!valid()) { BOOST_THROW_EXCEPTION(packaged_task_uninitialized()); }
            obtained_ = true;
            return future<R>(task_);
        }

        void operator()(Args&&... args)
        {
            if (!valid()) { BOOST_THROW_EXCEPTION(packaged_task_uninitialized()); }
            task_->run(std::forward<Args>(args)...);
        }
        
        void reset()
        {
            if (!valid()) { BOOST_THROW_EXCEPTION(packaged_task_uninitialized()); }
            obtained_ = false;
            task_->reset();
        }
    };
    
    template<typename Signature>
    void swap(packaged_task<Signature> &l, packaged_task<Signature> &r)
    { l.swap(r); }
    
}}

namespace fibio {
    using fibers::packaged_task;
}

#endif
