#ifndef BOOST_AURA_ATTRIBUTE_HPP_INCLUDED
#define BOOST_AURA_ATTRIBUTE_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/thread_affinity.hpp>

namespace boost { namespace aurae {

template <class T, thread_affinity Affinity = thread_affinity::emitting_thread>
struct attribute
{
    using value_type = T;
    static constexpr thread_affinity affinity = Affinity;
};

} }

#endif
