//
//  task_object.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-29.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibio_fibers_future_detail_task_object_hpp
#define fibio_fibio_fibers_future_detail_task_object_hpp

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <fibio/fibers/future/detail/task_base.hpp>

namespace fibio { namespace fibers { namespace detail {
    template< typename Fn, typename Allocator, typename R >
    class task_object : public task_base< R >
    {
    public:
        typedef typename Allocator::template rebind<
        task_object< Fn, Allocator, R >
        >::other                                      allocator_t;
        
        task_object( Fn && fn, allocator_t const& alloc) :
        task_base< R >(),
        fn_( std::forward< Fn >( fn) ), alloc_( alloc)
        {}
        
        void run()
        {
            try
            {
                this->set_value( fn_() );
            }
            catch (...)
            {
                this->set_exception(std::current_exception() );
            }
        }
        
    protected:
        void deallocate_future()
        { destroy_( alloc_, this); }
        
    private:
        Fn                  fn_;
        allocator_t         alloc_;
        
        static void destroy_( allocator_t & alloc, task_object * p)
        {
            alloc.destroy( p);
            alloc.deallocate( p, 1);
        }
    };
    
    template< typename Fn, typename Allocator >
    class task_object< Fn, Allocator, void > : public task_base< void >
    {
    public:
        typedef typename Allocator::template rebind<
        task_object< Fn, Allocator, void >
        >::other                                      allocator_t;
        
        task_object( Fn && fn, allocator_t const& alloc) :
        task_base< void >(),
        fn_( std::forward< Fn >( fn) ), alloc_( alloc)
        {}
        
        void run()
        {
            try
            {
                fn_();
                this->set_value();
            }
            catch (...)
            {
                this->set_exception(std::current_exception() );
            }
        }
        
    protected:
        void deallocate_future()
        { destroy_( alloc_, this); }
        
    private:
        Fn                  fn_;
        allocator_t         alloc_;
        
        static void destroy_( allocator_t & alloc, task_object * p)
        {
            alloc.destroy( p);
            alloc.deallocate( p, 1);
        }
    };
}}} // End of namespace fibio::fibers::detail

#endif
