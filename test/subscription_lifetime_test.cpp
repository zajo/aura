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
    int subscription_count;

    class subscription_counter
    {
        subscription_counter(subscription_counter const &);
        subscription_counter & operator=(subscription_counter const &);
    public:
        subscription_counter()
        {
            ++subscription_count;
        }
        ~subscription_counter()
        {
            --subscription_count;
        }
    };

    void test_owned_subscriptions()
    {
        BOOST_TEST_EQ(subscription_count, 0);
        {
            std::shared_ptr<int> sm = std::make_shared<int>(42);
            std::shared_ptr<aura::subscription> c1 = aura(&sm).subscribe<my_event>([](){ });
            std::shared_ptr<aura::subscription> c2 = aura(sm).subscribe<my_event>([](){ });
            c1->set_user_data(std::make_shared<subscription_counter>());
            c2->set_user_data(std::make_shared<subscription_counter>());
            BOOST_TEST_EQ(subscription_count, 2);
            sm.reset();
            BOOST_TEST_EQ(subscription_count, 2);
        }
         BOOST_TEST_EQ(subscription_count, 0);
    }

    void test_reset_subscription()
    {
        int count1 = 0, count2 = 0;
        my_source_type s1;
        my_source_type s2;
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<my_event>([&count1]() { ++count1; });
        std::shared_ptr<aura::subscription> c2 = aura(&s2).subscribe<my_event>([&count2]() { ++count2; });
        std::shared_ptr<aura::subscription> c3 = aura(&s1).subscribe<my_event>([&count1]() { ++count1; });
        std::shared_ptr<aura::subscription> c4 = aura(&s2).subscribe<my_event>([&count2]() { ++count2; });
        std::shared_ptr<aura::subscription> c5 = aura(&s1).subscribe<my_event>([&count1]() { ++count1; });
        std::shared_ptr<aura::subscription> c6 = aura(&s2).subscribe<my_event>([&count2]() { ++count2; });
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 3);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 3);
        BOOST_TEST_EQ(count2, 3);
        c1.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 3);
        c2.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        c3.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 2);
        c4.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        c5.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 1);
        c6.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
    }

    void test_reset_lifetime()
    {
        int count1 = 0, count2 = 0;
        my_source_type s1;
        my_source_type s2;
        std::shared_ptr<int> lt1(new int(1));
        std::shared_ptr<int> lt2(new int(2));
        std::shared_ptr<int> lt3(new int(3));
        std::shared_ptr<int> lt4(new int(4));
        std::shared_ptr<int> lt5(new int(5));
        std::shared_ptr<int> lt6(new int(6));
        (void) boost::aurae::persist(aura(&s1).subscribe<my_event>(lt1, [&count1](int * x) { BOOST_TEST_EQ(*x, 1); ++count1; }));
        (void) boost::aurae::persist(aura(&s2).subscribe<my_event>(lt2, [&count2](int * x) { BOOST_TEST_EQ(*x, 2); ++count2; }));
        (void) boost::aurae::persist(aura(&s1).subscribe<my_event>(lt3, [&count1](int * x) { BOOST_TEST_EQ(*x, 3); ++count1; }));
        (void) boost::aurae::persist(aura(&s2).subscribe<my_event>(lt4, [&count2](int * x) { BOOST_TEST_EQ(*x, 4); ++count2; }));
        (void) boost::aurae::persist(aura(&s1).subscribe<my_event>(lt5, [&count1](int * x) { BOOST_TEST_EQ(*x, 5); ++count1; }));
        (void) boost::aurae::persist(aura(&s2).subscribe<my_event>(lt6, [&count2](int * x) { BOOST_TEST_EQ(*x, 6); ++count2; }));
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 3);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 3);
        BOOST_TEST_EQ(count2, 3);
        lt1.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 3);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 3);
        lt2.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        lt3.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 2);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 2);
        lt4.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        lt5.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 1);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 1);
        lt6.reset();
        count1 = count2 = 0;
        BOOST_TEST_EQ(aura(&s1).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&s2).emit<my_event>(), 0);
        BOOST_TEST_EQ(count1, 0);
        BOOST_TEST_EQ(count2, 0);
    }

    struct null_deleter { void  operator()(void const *) { } };

    void test_source_expiring_before_subscription()
    {
        int count = 0;
        my_source_type s;
        std::shared_ptr<my_source_type> s1(&s, null_deleter());
        (void) boost::aurae::persist(aura(s1).subscribe<my_event>([&count](){ ++count; }));
        s1.reset();
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s).emit<my_event>(), 0);
    }
}

int main(int argc, char const * argv[])
{
    test_owned_subscriptions();
    test_reset_subscription();
    test_reset_lifetime();
    test_source_expiring_before_subscription();
    return boost::report_errors();
}
