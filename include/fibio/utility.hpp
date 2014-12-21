//
//  utility.hpp
//  fibio
//
//  Created by Chen Xu on 14/12/8.
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef utility_hpp
#define utility_hpp

#include <functional>
#include <tuple>
#include <type_traits>

namespace utility {
    template <std::size_t...> struct tuple_indices {};
    template <std::size_t Sp, class IntTuple, std::size_t Ep> struct make_indices_imp;
    template <std::size_t Sp, std::size_t... Indices, std::size_t Ep>
    struct make_indices_imp<Sp, tuple_indices<Indices...>, Ep>
    { typedef typename make_indices_imp<Sp+1, tuple_indices<Indices..., Sp>, Ep>::type type; };
    
    template <std::size_t Ep, std::size_t... Indices>
    struct make_indices_imp<Ep, tuple_indices<Indices...>, Ep>
    { typedef tuple_indices<Indices...> type; };
    
    template <std::size_t Ep, std::size_t Sp = 0>
    struct make_tuple_indices {
        static_assert(Sp <= Ep, "make_tuple_indices input error");
        typedef typename make_indices_imp<Sp, tuple_indices<>, Ep>::type type;
    };
    
    // invoke
    template <class Fp, class A0, class... Args>
    inline auto invoke(Fp&& f, A0&& a0, Args&&... args)
    -> decltype((std::forward<A0>(a0).*f)(std::forward<Args>(args)...))
    { return (std::forward<A0>(a0).*f)(std::forward<Args>(args)...); }
    
    template <class Fp, class A0, class... Args>
    inline auto invoke(Fp&& f, A0&& a0, Args&&... args)
    -> decltype(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...))
    { return ((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...); }
    
    template <class Fp, class A0>
    inline auto invoke(Fp&& f, A0&& a0)
    -> decltype(std::forward<A0>(a0).*f)
    { return std::forward<A0>(a0).*f; }
    
    template <class Fp, class A0>
    inline auto invoke(Fp&& f, A0&& a0)
    -> decltype((*std::forward<A0>(a0)).*f)
    { return (*std::forward<A0>(a0)).*f; }
    
    template <class Fp, class... Args>
    inline auto invoke(Fp&& f, Args&&... args)
    -> decltype(std::forward<Fp>(f)(std::forward<Args>(args)...))
    { return std::forward<Fp>(f)(std::forward<Args>(args)...); }
    
    /// decay_copy
    template <class T>
    typename std::decay<T>::type decay_copy(T&& t)
    { return std::forward<T>(t); }
    
    // make_function
    template<typename T> struct remove_class { };
    template<typename C, typename R, typename... A>
    struct remove_class<R(C::*)(A...)> { using type = R(A...); };
    template<typename C, typename R, typename... A>
    struct remove_class<R(C::*)(A...) const> { using type = R(A...); };
    template<typename C, typename R, typename... A>
    struct remove_class<R(C::*)(A...) volatile> { using type = R(A...); };
    template<typename C, typename R, typename... A>
    struct remove_class<R(C::*)(A...) const volatile> { using type = R(A...); };
    
    template<typename T>
    struct get_signature_impl
    { using type = typename remove_class<decltype(&std::remove_reference<T>::type::operator())>::type; };
    template<typename R, typename... A>
    struct get_signature_impl<R(A...)> { using type = R(A...); };
    template<typename R, typename... A>
    struct get_signature_impl<R(&)(A...)> { using type = R(A...); };
    template<typename R, typename... A>
    struct get_signature_impl<R(*)(A...)> { using type = R(A...); };
    template<typename T> using get_signature = typename get_signature_impl<T>::type;
    template<typename F> using make_function_type = std::function<get_signature<F>>;
    template<typename F> make_function_type<F> make_function(F &&f)
    { return make_function_type<F>(std::forward<F>(f)); }

    // Function traits
    template<typename T>
    struct function_traits {
        typedef typename remove_class<decltype(&std::remove_reference<T>::type::operator())>::type call_type;
        static constexpr size_t arity=function_traits<call_type>::arity;
        using result_type = typename function_traits<call_type>::result_type ;
        using arguments_tuple = typename function_traits<call_type>::arguments_tuple;
    };
    template<typename R, typename... A>
    struct function_traits<R(A...)> {
        enum { arity=sizeof...(A) };
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename... A>
    struct function_traits<R(&)(A...)> {
        typedef R result_type;
        static constexpr size_t arity=sizeof...(A);
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename... A>
    struct function_traits<R(*)(A...)> {
        static constexpr size_t arity=sizeof...(A);
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename C, typename... A>
    struct function_traits<R(C::*)(A...)> {
        static constexpr size_t arity=sizeof...(A);
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename C, typename... A>
    struct function_traits<R(C::*)(A...) const> {
        static constexpr size_t arity=sizeof...(A);
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename C, typename... A>
    struct function_traits<R(C::*)(A...) volatile> {
        static constexpr size_t arity=sizeof...(A);
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
    template<typename R, typename C, typename... A>
    struct function_traits<R(C::*)(A...) const volatile> {
        static constexpr size_t arity=sizeof...(A);
        typedef R result_type;
        typedef std::tuple<A...> arguments_tuple;
        template<size_t N>
        struct arg {
            typedef typename std::tuple_element<N, arguments_tuple>::type type;
        };
    };
}

#endif
