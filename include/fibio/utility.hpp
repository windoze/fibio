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

template <bool Value>
using bool_t = typename std::integral_constant<bool, Value>::type;
template <bool... Values>
struct and_;
template <>
struct and_<> : bool_t<true>
{
};
template <bool... Values>
struct and_<true, Values...> : and_<Values...>
{
};
template <bool... Values>
struct and_<false, Values...> : bool_t<false>
{
};
template <bool... Values>
struct or_;
template <>
struct or_<> : bool_t<false>
{
};
template <bool... Values>
struct or_<false, Values...> : or_<Values...>
{
};
template <bool... Values>
struct or_<true, Values...> : bool_t<true>
{
};

template <std::size_t...>
struct tuple_indices
{
};
template <std::size_t Sp, class IntTuple, std::size_t Ep>
struct make_indices_imp;
template <std::size_t Sp, std::size_t... Indices, std::size_t Ep>
struct make_indices_imp<Sp, tuple_indices<Indices...>, Ep>
{
    typedef typename make_indices_imp<Sp + 1, tuple_indices<Indices..., Sp>, Ep>::type type;
};

template <std::size_t Ep, std::size_t... Indices>
struct make_indices_imp<Ep, tuple_indices<Indices...>, Ep>
{
    typedef tuple_indices<Indices...> type;
};

template <std::size_t Ep, std::size_t Sp = 0>
struct make_tuple_indices
{
    static_assert(Sp <= Ep, "make_tuple_indices input error");
    typedef typename make_indices_imp<Sp, tuple_indices<>, Ep>::type type;
};

#if defined(_WIN32) && (_MSC_VER >= 1900)
// Visual C++ 2015 supports C++17 std::invoke
using std::invoke;
#else

// invoke
template <class Fp, class A0, class... Args>
inline auto invoke(Fp&& f, A0&& a0, Args&&... args)
    -> decltype((std::forward<A0>(a0).*f)(std::forward<Args>(args)...))
{
    return (std::forward<A0>(a0).*f)(std::forward<Args>(args)...);
}

template <class Fp, class A0, class... Args>
inline auto invoke(Fp&& f, A0&& a0, Args&&... args)
    -> decltype(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...))
{
    return ((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...);
}

template <class Fp, class A0>
inline auto invoke(Fp&& f, A0&& a0) -> decltype(std::forward<A0>(a0).*f)
{
    return std::forward<A0>(a0).*f;
}

template <class Fp, class A0>
inline auto invoke(Fp&& f, A0&& a0) -> decltype((*std::forward<A0>(a0)).*f)
{
    return (*std::forward<A0>(a0)).*f;
}

template <class Fp, class... Args>
inline auto invoke(Fp&& f, Args&&... args)
    -> decltype(std::forward<Fp>(f)(std::forward<Args>(args)...))
{
    return std::forward<Fp>(f)(std::forward<Args>(args)...);
}

#endif

/// decay_copy
template <class T>
typename std::decay<T>::type decay_copy(T&& t)
{
    return std::forward<T>(t);
}

// make_function
template <typename T>
struct remove_class
{
};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...)>
{
    using type = R(A...);
};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...) const>
{
    using type = R(A...);
};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...) volatile>
{
    using type = R(A...);
};
template <typename C, typename R, typename... A>
struct remove_class<R (C::*)(A...) const volatile>
{
    using type = R(A...);
};

template <typename T>
struct get_signature_impl
{
    using type = typename remove_class<decltype(&std::remove_reference<T>::type::operator())>::type;
};
template <typename R, typename... A>
struct get_signature_impl<R(A...)>
{
    using type = R(A...);
};
template <typename R, typename... A>
struct get_signature_impl<R(&)(A...)>
{
    using type = R(A...);
};
template <typename R, typename... A>
struct get_signature_impl<R (*)(A...)>
{
    using type = R(A...);
};
template <typename T>
using get_signature = typename get_signature_impl<T>::type;
template <typename F>
using make_function_type = std::function<get_signature<F>>;

template <typename F>
make_function_type<F> make_function(F&& f)
{
    return make_function_type<F>(std::forward<F>(f));
}

// Function traits
template <typename T>
struct function_traits
{
    typedef typename remove_class<decltype(&std::remove_reference<T>::type::operator())>::type
        call_type;
    enum
    {
        arity = function_traits<call_type>::arity
    };
    using result_type = typename function_traits<call_type>::result_type;
    using arguments_tuple = typename function_traits<call_type>::arguments_tuple;
    template <size_t N>
    using arg = typename function_traits<call_type>::template arg<N>;
};
template <typename R, typename... A>
struct function_traits<R(A...)>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename... A>
struct function_traits<R(&)(A...)>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename... A>
struct function_traits<R (*)(A...)>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...)>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...) const>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...) volatile>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};
template <typename R, typename C, typename... A>
struct function_traits<R (C::*)(A...) const volatile>
{
    enum
    {
        arity = sizeof...(A)
    };
    typedef R result_type;
    typedef std::tuple<A...> arguments_tuple;
    template <size_t N>
    struct arg
    {
        typedef typename std::tuple_element<N, arguments_tuple>::type type;
    };
};

template <class T>
inline std::exception_ptr copy_exception(T const& e)
{
    try {
        throw e;
    } catch (...) {
        return std::current_exception();
    }
}

} // End of namespace utility

#endif
