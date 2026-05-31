// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"

using aura = boost::aurae::aura;

namespace
{
    struct my_source_type {};
    struct my_event: aura::event<void()> {};

    void test_subscription_expiration_during_emit1()
    {
        my_source_type s;
        int count0 = 0, count1 = 0, count2 = 0;
        std::shared_ptr<aura::subscription> c1;
        std::shared_ptr<aura::subscription> c0 = aura(&s).subscribe<my_event>(
            [&c1, &count0]()
            {
                if (++count0 == 3)
                    c1.reset();
            });
        c1 = aura(&s).subscribe<my_event>([&count1]() { ++count1; });
        std::shared_ptr<aura::subscription> c2 = aura(&s).subscribe<my_event>([&count2]() { ++count2; });
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 3);
        BOOST_TEST(c1);
        BOOST_TEST_EQ(count0, 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 3);
        BOOST_TEST(c1);
        BOOST_TEST_EQ(count0, 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 2);
        BOOST_TEST(!c1);
        BOOST_TEST_EQ(count0, 3);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 3);
    }

    void test_subscription_expiration_during_emit2()
    {
        my_source_type s;
        int count0 = 0, count1 = 0, count2 = 0;
        std::shared_ptr<int> lifetime(new int(42));
        std::shared_ptr<aura::subscription> c0 = aura(&s).subscribe<my_event>(
            [&lifetime, &count0]()
            {
                if (++count0 == 3)
                    lifetime.reset();
            });
        (void) boost::aurae::persist(aura(&s).subscribe<my_event>(lifetime, [&count1](int * x) { BOOST_TEST_EQ(*x, 42); ++count1; }));
        std::shared_ptr<aura::subscription> c2 = aura(&s).subscribe<my_event>([&count2]() { ++count2; });
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 3);
        BOOST_TEST(lifetime);
        BOOST_TEST_EQ(count0, 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 3);
        BOOST_TEST(lifetime);
        BOOST_TEST_EQ(count0, 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 2);
        BOOST_TEST(!lifetime);
        BOOST_TEST_EQ(count0, 3);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 3);
    }

    struct null_deleter { template <class T> void operator()(T *) { } };

    void source_address_reuse_test()
    {
        my_source_type s2;
        int n1 = 0;
        std::shared_ptr<my_source_type> s1(&s2, null_deleter());
        (void) boost::aurae::persist(aura(s1).subscribe<my_event>([&n1]() { ++n1; }));
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(n1, 1);
        s1.reset();
        int n2 = 0;
        std::shared_ptr<aura::subscription> c2 = aura(&s2).subscribe<my_event>([&n2]() { ++n2; });
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(n1, 1);
        BOOST_TEST_EQ(n2, 1);
        c2.reset();
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 0);
        BOOST_TEST_EQ(n1, 1);
        BOOST_TEST_EQ(n2, 1);
    }
}

int main(int argc, char const * argv[])
{
    test_subscription_expiration_during_emit1();
    test_subscription_expiration_during_emit2();
    source_address_reuse_test();
    return boost::report_errors();
}
