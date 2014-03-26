//
//  make_tuple_indices.hpp
//  fibio
//
//  Created by Chen Xu on 14-3-21.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_fibers_detail_make_tuple_indices_hpp
#define fibio_fibers_detail_make_tuple_indices_hpp

#include <functional>
#include <tuple>

namespace fibio { namespace detail {
    template <std::size_t...>
    struct indices;
    
    template <std::size_t N, typename Indices, typename... Types>
    struct make_indices_impl;
    
    template <std::size_t N, std::size_t... Indices, typename Type, typename... Types >
    struct make_indices_impl<N, indices<Indices...>, Type, Types...> {
        typedef typename make_indices_impl<N+1, indices<Indices...,N>, Types... >::type type;
    };
    
    template <std::size_t N, std::size_t... Indices>
    struct make_indices_impl<N, indices<Indices...>> {
        typedef indices<Indices...> type;
    };
    
    template <std::size_t N, typename... Types>
    struct make_indices {
        typedef typename make_indices_impl<0, indices<>, Types...>::type type;
    };
    
    template <typename Op, typename... Args>
    auto apply(Op&& op, Args&&... args) -> typename std::result_of<Op(Args...)>::type {
        return op(args...);
    }
    
    template <typename Indices>
    struct apply_tuple_impl;
    
    template <template <std::size_t...> class I, std::size_t... Indices>
    struct apply_tuple_impl<I<Indices...>> {
        template <typename Op, typename... OpArgs, template <typename...> class T = std::tuple>
        static auto apply_tuple(Op&& op, T<OpArgs...>&& t) -> typename std::result_of<Op(OpArgs...)>::type {
            return op( std::forward<OpArgs>(std::get<Indices>(t))... );
        }
    };
    
    template <typename Op, typename... OpArgs, typename Indices = typename make_indices<0, OpArgs...>::type, template <typename...> class T = std::tuple>
    auto apply_tuple(Op&& op, T<OpArgs...>&& t) -> typename std::result_of<Op(OpArgs...)>::type{
        return apply_tuple_impl<Indices>::apply_tuple(std::forward<Op>(op),
                                                      std::forward<T<OpArgs...>>(t));
    }
    
    template <class T>
    inline typename std::decay<T>::type decay_copy(T&& t) {
        return std::forward<T>(t);
    }
    
    template<typename Fn, typename... Args>
    auto wrap(Fn &&fn, Args&&... args) -> std::function<typename std::result_of<Fn(Args...)>::type()>
    {
        typedef std::tuple<typename std::decay<Fn>::type> FP;
        FP *fp=new FP(decay_copy(std::forward<Fn>(fn)));
        typedef std::tuple<typename std::decay<Args>::type...> TP;
        TP *p=new TP(decay_copy(std::forward<Args>(args))...);
        return [fp, p](){
            std::unique_ptr<FP> ufp(fp);
            std::unique_ptr<TP> utp(p);
            return apply_tuple(std::get<0>(*fp), std::move(*p));
        };
    }
}}  // End of namespace

#endif
