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
#include <boost/static_assert.hpp>
#include <fibio/utility.hpp>

namespace fibio { namespace fibers { namespace detail {
    /// fiber_data_base
    struct fiber_data_base
    {
        virtual ~fiber_data_base(){}
        virtual void run()=0;
    };
    
    /// fiber_data
    template<typename F, class... ArgTypes>
    class fiber_data : public fiber_data_base
    {
    public:
        fiber_data(F&& f_, ArgTypes&&... args_)
        : fp(std::forward<F>(f_), std::forward<ArgTypes>(args_)...)
        {}

        template <std::size_t... Indices>
        void run2(utility::tuple_indices<Indices...>)
        { utility::invoke(std::move(std::get<0>(fp)), std::move(std::get<Indices>(fp))...); }

        void run()
        {
            typedef typename utility::make_tuple_indices<std::tuple_size<std::tuple<F, ArgTypes...> >::value, 1>::type index_type;
            run2(index_type());
        }
        
    private:
        /// Non-copyable
        fiber_data(const fiber_data&)=delete;
        void operator=(const fiber_data&)=delete;
        std::tuple<typename std::decay<F>::type, typename std::decay<ArgTypes>::type...> fp;
    };
    
    /**
     * make_fiber_data
     *
     * wrap entry function and arguments into a tuple
     */
    template<typename F, class... ArgTypes>
    inline fiber_data_base *make_fiber_data(F&& f, ArgTypes&&... args)
    {
        return new fiber_data<typename std::remove_reference<F>::type, ArgTypes...>(std::forward<F>(f),
                                                                                    std::forward<ArgTypes>(args)...);
    }
}}} // End of namespace fibio::fibers::detail
#endif
