#ifndef BOOST_AURA_EVENT_HPP_INCLUDED
#define BOOST_AURA_EVENT_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/thread_affinity.hpp>
#include <type_traits>

namespace boost { namespace aurae {

template <class Signature, thread_affinity Affinity = thread_affinity::emitting_thread>
struct event;

template <class R, class... A, thread_affinity Affinity>
struct event<R(A...), Affinity>
{
    static_assert(std::is_same<R, void>::value, "aura::event signature must have a void return type");
    using signature = void(A...);
    static int const arity = sizeof...(A);
    static constexpr thread_affinity affinity = Affinity;
};

} }

#endif
