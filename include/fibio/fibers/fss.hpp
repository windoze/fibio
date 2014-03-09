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

    template <typename T>
    class fiber_specific_ptr {
    private:
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
        
    public:
        typedef T element_type;
        
        fiber_specific_ptr(): cleanup(detail::heap_new<delete_data>(),
                                      detail::do_heap_delete<delete_data>())
        {}
        
        explicit fiber_specific_ptr(void (*func_)(T*)) {
            if(func_) {
                cleanup.reset(detail::heap_new<run_custom_cleanup_function>(func_),
                              detail::do_heap_delete<run_custom_cleanup_function>());
            }
        }
        
        ~fiber_specific_ptr() {
            detail::set_fss_data(this,std::shared_ptr<detail::fss_cleanup_function>(),0,true);
        }
        
        T* get() const {
            return static_cast<T*>(detail::get_fss_data(this));
        }
        
        T* operator->() const {
            return get();
        }
        
        T& operator*() const {
            return *get();
        }
        
        T* release() {
            T* const temp=get();
            detail::set_fss_data(this,std::shared_ptr<detail::fss_cleanup_function>(),0,false);
            return temp;
        }
        
        void reset(T* new_value=0) {
            T* const current_value=get();
            if(current_value!=new_value) {
                detail::set_fss_data(this,cleanup,new_value,true);
            }
        }
    };
}}  // End of namespace fibio::fibers

namespace fibio {
    template<typename T> using fiber_specific_ptr = fibers::fiber_specific_ptr<T>;
}   // End of namespace fibio

#endif
