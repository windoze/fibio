//
//  make_tuple_indices.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-21.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_make_tuple_indices_hpp
#define fibio_fibers_detail_make_tuple_indices_hpp

#include <memory>
#include <boost/thread/detail/make_tuple_indices.hpp>
#include <boost/thread/detail/invoke.hpp>

namespace fibio { namespace fibers { namespace detail {
    template <class T>
    typename std::decay<T>::type decay_copy(T&& t)
    {
        return std::forward<T>(t);
    }

    struct fiber_data_base : std::enable_shared_from_this<fiber_data_base>
    {
        virtual ~fiber_data_base(){}
        virtual void run()=0;
    };
    
    typedef std::shared_ptr<fiber_data_base> fiber_data_ptr;
    
    template<typename F, class ...ArgTypes>
    class fiber_data : public fiber_data_base
    {
    public:
        fiber_data(F &&f_, ArgTypes&&... args_)
        : fp(std::forward<F>(f_), std::forward<ArgTypes>(args_)...)
        {}

        fiber_data(const fiber_data&)=delete;
        void operator=(const fiber_data&)=delete;
        
        template <std::size_t ...Indices>
        void run2(boost::detail::tuple_indices<Indices...>)
        {
            boost::detail::invoke(std::move(std::get<0>(fp)), std::move(std::get<Indices>(fp))...);
        }

        void run()
        {
            typedef typename boost::detail::make_tuple_indices<std::tuple_size<std::tuple<F, ArgTypes...> >::value, 1>::type index_type;
            run2(index_type());
        }
        
    private:
        std::tuple<typename std::decay<F>::type, typename std::decay<ArgTypes>::type...> fp;
    };
    
    template<typename F, class ...ArgTypes>
    inline fiber_data_ptr make_fiber_data(F &&f, ArgTypes&&... args)
    {
        return std::make_shared<fiber_data<typename std::remove_reference<F>::type, ArgTypes...>>
        (boost::forward<F>(f), boost::forward<ArgTypes>(args)...);
    }
}}} // End of namespace fibio::fibers::detail
#endif
