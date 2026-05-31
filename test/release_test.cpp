// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

struct my_event: aura::event<void()> {};
struct my_source {};

int main(int argc, char const * argv[])
{
    int meta_counter = 0;
    std::shared_ptr<aura::subscription> mc = aura::meta().subscribe<aura::meta::subscribed<my_event>>(
        [&meta_counter](aura::subscription & c, unsigned context)
        {
            if (context & aura::meta::context_flags::subscribing)
                ++meta_counter;
            else
                --meta_counter;
        });

    std::shared_ptr<my_source> s1 = std::make_shared<my_source>();
    int emit_counter1 = 0;
    BOOST_TEST_EQ(meta_counter, 0);
    std::weak_ptr<aura::subscription> c1 = aurae::persist(aura(s1).subscribe<my_event>(
        [&emit_counter1]()
    {
        ++emit_counter1;
    }));
    BOOST_TEST(!c1.expired());

    std::shared_ptr<my_source> s2 = std::make_shared<my_source>();
    int emit_counter2 = 0;
    BOOST_TEST_EQ(meta_counter, 1);
    std::weak_ptr<aura::subscription> c2 = aurae::persist(aura(s2).subscribe<my_event>(
        [&emit_counter2]()
    {
        ++emit_counter2;
    }));
    BOOST_TEST(!c2.expired());

    BOOST_TEST_EQ(meta_counter, 2);
    BOOST_TEST_EQ(emit_counter1, 0);
    BOOST_TEST_EQ(emit_counter2, 0);
    aura(s1.get()).emit<my_event>();
    BOOST_TEST_EQ(emit_counter1, 1);
    BOOST_TEST_EQ(emit_counter2, 0);
    aura(s2.get()).emit<my_event>();
    BOOST_TEST_EQ(emit_counter1, 1);
    BOOST_TEST_EQ(emit_counter2, 1);

    s1.reset();
    s2.reset();
    BOOST_TEST_EQ(meta_counter, 2);
    {
        BOOST_TEST(!c1.expired());
        BOOST_TEST(!c2.expired());
        std::shared_ptr<aura::subscription> c = aura(&meta_counter).subscribe<my_event>([](){ });
        BOOST_TEST(c1.expired());
        BOOST_TEST(c2.expired());
        BOOST_TEST_EQ(meta_counter, 1);
    }

    BOOST_TEST_EQ(meta_counter, 0);

    {
        int src;
        std::shared_ptr<aura::subscription> r = aurae::release(aurae::persist(aura(&src).subscribe<my_event>([](){ })));
        BOOST_TEST(r);
        BOOST_TEST(r.use_count() == 1);
    }

    return boost::report_errors();
}
