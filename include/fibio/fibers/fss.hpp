//
//  fss.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-9.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fss_hpp
#define fibio_fss_hpp

#include <memory>

namespace fibio { namespace fibers {
    namespace detail {
        struct fss_cleanup_function{
            virtual ~fss_cleanup_function() {}
            virtual void operator()(void* data)=0;
        };
        
        template<typename T>
        inline T* heap_new()
        { return new T(); }
        
        template<typename T>
        inline void heap_delete(T* data)
        { delete data; }
        
        template<typename T>
        struct do_heap_delete {
            void operator()(T* data) const
            { detail::heap_delete(data); }
        };

        void set_fss_data(void const* key,std::shared_ptr<fss_cleanup_function> func,void* fss_data,bool cleanup_existing);
        void* get_fss_data(void const* key);
    }   // End of namespace detail

    /**
     * Fiber local storage allows multi-fiber applications to have a separate instance of a given data item for each fiber.
     */
    template <typename T>
    class fiber_specific_ptr {
    public:
        typedef T element_type;
        
        /**
         * Construct a `fiber_specific_ptr` object for storing a pointer
         * to an object of type `T` specific to each fiber. The default
         * delete-based cleanup function will be used to destroy any
         * fiber-local objects when `reset()` is called, or the fiber
         * exits.
         */
        fiber_specific_ptr()
        : cleanup(detail::heap_new<delete_data>(),
                  detail::do_heap_delete<delete_data>())
        {}
        
        /**
         * Construct a `fiber_specific_ptr` object for storing a pointer
         * to an object of type `T` specific to each fiber. The supplied
         * cleanup_function will be used to destroy any fiber-local
         * objects when `reset()` is called, or the fiber exits.
         */
        explicit fiber_specific_ptr(void (*cleanup_function)(T*)) {
            if(cleanup_function) {
                cleanup.reset(detail::heap_new<run_custom_cleanup_function>(cleanup_function),
                              detail::do_heap_delete<run_custom_cleanup_function>());
            }
        }
        
        /**
         * Calls `this->reset()` to clean up the associated value for the
         * current fiber, and destroys `*this`.
         */
        ~fiber_specific_ptr() {
            detail::set_fss_data(this,std::shared_ptr<detail::fss_cleanup_function>(),0,true);
        }
        
        /**
         * Returns the pointer associated with the current thread.
         */
        T* get() const {
            return static_cast<T*>(detail::get_fss_data(this));
        }
        
        /**
         * Returns `this->get()`
         */
        T* operator->() const {
            return get();
        }
        
        /**
         * Returns `*(this->get())`
         */
        T& operator*() const {
            return *get();
        }
        
        /**
         * Return `this->get()` and store `NULL` as the pointer associated
         * with the current fiber without invoking the cleanup function.
         */
        T* release() {
            T* const temp=get();
            detail::set_fss_data(this,std::shared_ptr<detail::fss_cleanup_function>(),0,false);
            return temp;
        }
        
        /**
         * If `this->get()!=new_value` and `this->get()` is non-NULL, invoke
         * `delete this->get()` or `cleanup_function(this->get())` as appropriate.
         * Store `new_value` as the pointer associated with the current fiber.
         */
        void reset(T* new_value=0) {
            T* const current_value=get();
            if(current_value!=new_value) {
                detail::set_fss_data(this,cleanup,new_value,true);
            }
        }
        
    private:
        /// non-copyable
        fiber_specific_ptr(fiber_specific_ptr&)=delete;
        fiber_specific_ptr& operator=(fiber_specific_ptr&)=delete;
        
        struct delete_data: detail::fss_cleanup_function {
            void operator()(void* data) {
                delete static_cast<T*>(data);
            }
        };
        
        struct run_custom_cleanup_function: detail::fss_cleanup_function {
            void (*cleanup_function)(T*);
            
            explicit run_custom_cleanup_function(void (*cleanup_function_)(T*)):
            cleanup_function(cleanup_function_)
            {}
            
            void operator()(void* data) {
                cleanup_function(static_cast<T*>(data));
            }
        };
        
        std::shared_ptr<detail::fss_cleanup_function> cleanup;
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    template<typename T> using fiber_specific_ptr = fibers::fiber_specific_ptr<T>;
}   // End of namespace fibio

#endif
