#ifndef BOOST_AURA_THREAD_AFFINITY_HPP_INCLUDED
#define BOOST_AURA_THREAD_AFFINITY_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

namespace boost { namespace aurae {

enum class thread_affinity
{
    emitting_thread,
    cross_thread
};

} }

#endif
