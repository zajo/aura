#ifndef BOOST_AURA_DETAIL_MP11_HPP_INCLUDED
#define BOOST_AURA_DETAIL_MP11_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski
// Copyright 2015, 2017 Peter Dimov.

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This header contains code copied from Boost MP11, implementing tuple_apply.
// This is the only part Aura needs from mp11, in order to not require C++17.

#include <boost/aura/config.hpp>
#include <tuple>
#include <utility>
#include <type_traits>

#if defined(__has_builtin)
#   if __has_builtin(__make_integer_seq)
#		define BOOST_AURA_HAS_MAKE_INTEGER_SEQ
#	endif
#endif

#define BOOST_AURA_CONSTEXPR constexpr

namespace boost { namespace aurae { namespace detail_mp11 {

template<class T, T... I> struct integer_sequence
{
};

#if defined(BOOST_AURA_HAS_MAKE_INTEGER_SEQ)

template<class T, T N> using make_integer_sequence = __make_integer_seq<integer_sequence, T, N>;

#else

namespace detail
{

template<bool C, class T, class E> struct iseq_if_c_impl;

template<class T, class E> struct iseq_if_c_impl<true, T, E>
{
    using type = T;
};

template<class T, class E> struct iseq_if_c_impl<false, T, E>
{
    using type = E;
};

template<bool C, class T, class E> using iseq_if_c = typename iseq_if_c_impl<C, T, E>::type;

template<class T> struct iseq_identity
{
    using type = T;
};

template<class S1, class S2> struct append_integer_sequence;

template<class T, T... I, T... J> struct append_integer_sequence<integer_sequence<T, I...>, integer_sequence<T, J...>>
{
    using type = integer_sequence< T, I..., ( J + sizeof...(I) )... >;
};

template<class T, T N> struct make_integer_sequence_impl;

template<class T, T N> struct make_integer_sequence_impl_
{
private:

    static_assert( N >= 0, "make_integer_sequence<T, N>: N must not be negative" );

    static T const M = N / 2;
    static T const R = N % 2;

    using S1 = typename make_integer_sequence_impl<T, M>::type;
    using S2 = typename append_integer_sequence<S1, S1>::type;
    using S3 = typename make_integer_sequence_impl<T, R>::type;
    using S4 = typename append_integer_sequence<S2, S3>::type;

public:

    using type = S4;
};

template<class T, T N> struct make_integer_sequence_impl: iseq_if_c<N == 0, iseq_identity<integer_sequence<T>>, iseq_if_c<N == 1, iseq_identity<integer_sequence<T, 0>>, make_integer_sequence_impl_<T, N> > >
{
};

} // namespace detail

template<class T, T N> using make_integer_sequence = typename detail::make_integer_sequence_impl<T, N>::type;

#endif // defined(BOOST_AURA_HAS_MAKE_INTEGER_SEQ)

template<std::size_t... I> using index_sequence = integer_sequence<std::size_t, I...>;

template<std::size_t N> using make_index_sequence = make_integer_sequence<std::size_t, N>;

////////////////////////////////////////

namespace detail
{

template<class F, class Tp, std::size_t... J> BOOST_AURA_CONSTEXPR auto tuple_apply_impl( F && f, Tp && tp, integer_sequence<std::size_t, J...> )
    -> decltype( std::forward<F>(f)( std::get<J>(std::forward<Tp>(tp))... ) )
{
    return std::forward<F>(f)( std::get<J>(std::forward<Tp>(tp))... );
}

} // namespace detail

template<class F, class Tp,
    class Seq = make_index_sequence<std::tuple_size<typename std::remove_reference<Tp>::type>::value>>
BOOST_AURA_CONSTEXPR auto tuple_apply( F && f, Tp && tp )
    -> decltype( detail::tuple_apply_impl( std::forward<F>(f), std::forward<Tp>(tp), Seq() ) )
{
    return detail::tuple_apply_impl( std::forward<F>(f), std::forward<Tp>(tp), Seq() );
}

} } }

#endif
