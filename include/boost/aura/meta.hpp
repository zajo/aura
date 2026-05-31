#ifndef BOOST_AURA_META_HPP_INCLUDED
#define BOOST_AURA_META_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/event.hpp>
#include <memory>

namespace boost { namespace aurae {

class subscription;
class attachment;
class muting;

struct meta
{
    template <class Event>
    struct subscribed:
        event<void(subscription &, unsigned)>
    {
    };

    template <class Attr>
    struct attached:
        event<void(attachment &, unsigned)>
    {
    };

    template <class Event>
    struct muted:
        event<void(muting &, unsigned)>
    {
    };

    struct context_flags
    {
        static unsigned const subscribing = 1;
        static unsigned const first_for_source = 2;
        static unsigned const last_for_source = 4;
    };

    template <class Event, class F>
    std::shared_ptr<subscription> subscribe(F) const;
};

} }

#endif
