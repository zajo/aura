// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/mute.hpp>
#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    template <class Event>
    bool source_muted(void const * s)
    {
        std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<Event>(false);
        return tled && aurae::detail::is_muted_(*tled, s);
    }
    struct my_event: aura::event<void()> {};
}

int main(int argc, char const * argv[])
{
    int s1 = 0, s2 = 0;
    BOOST_TEST(!source_muted<my_event>(&s1));
    BOOST_TEST(!source_muted<my_event>(&s2));
    std::shared_ptr<aura::muting> b1 = aura(&s1).mute<my_event>();
    BOOST_TEST(source_muted<my_event>(&s1));
    BOOST_TEST(!source_muted<my_event>(&s2));
    std::shared_ptr<aura::muting> b2 = aura(&s2).mute<my_event>();
    BOOST_TEST(source_muted<my_event>(&s1));
    BOOST_TEST(source_muted<my_event>(&s2));
    int count = 0;
    std::shared_ptr<aura::subscription> c = aura(&s1).subscribe<my_event>([&count]() { ++count; });
    BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 0);
    BOOST_TEST_EQ(count, 0);
    b1.reset();
    BOOST_TEST(!source_muted<my_event>(&s1));
    BOOST_TEST(source_muted<my_event>(&s2));
    BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 1);
    BOOST_TEST_EQ(count, 1);
    b2.reset();
    BOOST_TEST(!source_muted<my_event>(&s1));
    BOOST_TEST(!source_muted<my_event>(&s2));
    return boost::report_errors();
}
