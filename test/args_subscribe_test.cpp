// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"
#include <string>

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{

    struct my_source_type {};

    //////////////////////////////////////////////////////////////////////////////////////////////////

    struct event1_a0: aura::event<void()> {};
    struct event2_a0: aura::event<void()> {};

    void test_a0()
    {
        int subscribe_count = 0;
        int count = 0;
        my_source_type s1;
        std::shared_ptr<aura::subscription> mc_c1 = aura::meta().subscribe<aura::meta::subscribed<event1_a0>>(
            [&s1, &subscribe_count](aura::subscription & c, unsigned context)
            {
                if (context & aura::meta::context_flags::subscribing)
                {
                    BOOST_TEST_EQ(&s1, c.source<my_source_type>().get());
                    ++subscribe_count;
                }
                else
                    --subscribe_count;
            });
        BOOST_TEST(mc_c1.use_count() == 1);
        BOOST_TEST_EQ(subscribe_count, 0);
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<event1_a0>(
            [&count]()
            {
                ++count;
            });
        BOOST_TEST_EQ(subscribe_count, 1);
        BOOST_TEST_EQ(aura(&s1).emit<event2_a0>(), 0);
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s1).emit<event1_a0>(), 1);
        BOOST_TEST_EQ(count, 1);
        c1.reset();
        BOOST_TEST_EQ(subscribe_count, 0);
        {
            std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<event1_a0>(false);
            BOOST_TEST(!std::weak_ptr<void>().owner_before(tled->sl_) && !tled->sl_.owner_before(std::weak_ptr<void>()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    struct event0_a1: aura::event<void(short)> {};
    struct event1_a1: aura::event<void(int)> {};
    struct event2_a1: aura::event<void(int)> {};

    void test_a1()
    {
        int subscribe_count = 0;
        int count = 0;
        my_source_type s1;
        std::shared_ptr<aura::subscription> mc_c1 = aura::meta().subscribe<aura::meta::subscribed<event1_a1>>(
            [&s1, &subscribe_count](aura::subscription & c, unsigned context)
            {
                if (context & aura::meta::context_flags::subscribing)
                {
                    BOOST_TEST_EQ(&s1, c.source<my_source_type>().get());
                    ++subscribe_count;
                }
                else
                    --subscribe_count;
            });
        BOOST_TEST(mc_c1.use_count() == 1);
        BOOST_TEST_EQ(subscribe_count, 0);
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<event1_a1>(
            [&count](int a1)
            {
                BOOST_TEST_EQ(a1, 42);
                ++count;
            });
        BOOST_TEST_EQ(subscribe_count, 1);
        BOOST_TEST_EQ(aura(&s1).emit<event2_a1>(42), 0);
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s1).emit<event1_a1>(42), 1);
        BOOST_TEST_EQ(count, 1);
        c1.reset();
        BOOST_TEST_EQ(subscribe_count, 0);
        {
            std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<event1_a1>(false);
            BOOST_TEST(!std::weak_ptr<void>().owner_before(tled->sl_) && !tled->sl_.owner_before(std::weak_ptr<void>()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    struct event0_a2: aura::event<void(int, double)> {};
    struct event1_a2: aura::event<void(int, float)> {};
    struct event2_a2: aura::event<void(int, float)> {};

    void test_a2()
    {
        int subscribe_count = 0;
        int count = 0;
        my_source_type s1;
        std::shared_ptr<aura::subscription> mc_c1 = aura::meta().subscribe<aura::meta::subscribed<event1_a2>>(
            [&s1, &subscribe_count](aura::subscription & c, unsigned context)
            {
                if (context & aura::meta::context_flags::subscribing)
                {
                    BOOST_TEST_EQ(&s1, c.source<my_source_type>().get());
                    ++subscribe_count;
                }
                else
                    --subscribe_count;
            });
        BOOST_TEST(mc_c1.use_count() == 1);
        BOOST_TEST_EQ(subscribe_count, 0);
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<event1_a2>(
            [&count](int a1, float a2)
            {
                BOOST_TEST_EQ(a1, 42);
                BOOST_TEST_EQ(a2, 42.42f);
                ++count;
            });
        BOOST_TEST_EQ(subscribe_count, 1);
        BOOST_TEST_EQ(aura(&s1).emit<event2_a2>(42, 42.42f), 0);
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s1).emit<event1_a2>(42, 42.42f), 1);
        BOOST_TEST_EQ(count, 1);
        c1.reset();
        BOOST_TEST_EQ(subscribe_count, 0);
        {
            std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<event1_a2>(false);
            BOOST_TEST(!std::weak_ptr<void>().owner_before(tled->sl_) && !tled->sl_.owner_before(std::weak_ptr<void>()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    struct event0_a3: aura::event<void(int, float, char const *)> {};
    struct event1_a3: aura::event<void(int, float, std::string const &)> {};
    struct event2_a3: aura::event<void(int, float, std::string const &)> {};

    void test_a3()
    {
        int subscribe_count = 0;
        int count = 0;
        my_source_type s1;
        std::shared_ptr<aura::subscription> mc_c1 = aura::meta().subscribe<aura::meta::subscribed<event1_a3>>(
            [&s1, &subscribe_count](aura::subscription & c, unsigned context)
            {
                if (context & aura::meta::context_flags::subscribing)
                {
                    BOOST_TEST_EQ(&s1, c.source<my_source_type>().get());
                    ++subscribe_count;
                }
                else
                    --subscribe_count;
            });
        BOOST_TEST(mc_c1.use_count() == 1);
        BOOST_TEST_EQ(subscribe_count, 0);
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<event1_a3>(
            [&count](int a1, float a2, std::string const & a3)
            {
                BOOST_TEST_EQ(a1, 42);
                BOOST_TEST_EQ(a2, 42.42f);
                BOOST_TEST_EQ(a3, "42");
                ++count;
            });
        BOOST_TEST_EQ(subscribe_count, 1);
        BOOST_TEST_EQ(aura(&s1).emit<event2_a3>(42, 42.42f, "42"), 0);
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s1).emit<event1_a3>(42, 42.42f, "42"), 1);
        BOOST_TEST_EQ(count, 1);
        c1.reset();
        BOOST_TEST_EQ(subscribe_count, 0);
        {
            std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<event1_a3>(false);
            BOOST_TEST(!std::weak_ptr<void>().owner_before(tled->sl_) && !tled->sl_.owner_before(std::weak_ptr<void>()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    struct event0_a4: aura::event<void(int, float, char const *, short &)> {};
    struct event1_a4: aura::event<void(int, float, std::string const &, short &)> {};
    struct event2_a4: aura::event<void(int, float, std::string const &, short &)> {};

    void test_a4()
    {
        int subscribe_count = 0;
        int count = 0;
        my_source_type s1;
        std::shared_ptr<aura::subscription> mc_c1 = aura::meta().subscribe<aura::meta::subscribed<event1_a4>>(
            [&s1, &subscribe_count](aura::subscription & c, unsigned context)
            {
                if (context & aura::meta::context_flags::subscribing)
                {
                    BOOST_TEST_EQ(&s1, c.source<my_source_type>().get());
                    ++subscribe_count;
                }
                else
                    --subscribe_count;
            });
        BOOST_TEST(mc_c1.use_count() == 1);
        BOOST_TEST_EQ(subscribe_count, 0);
        short a4 = 42;
        std::shared_ptr<aura::subscription> c1 = aura(&s1).subscribe<event1_a4>(
            [&count, &a4](int a1, float a2, std::string const & a3, short & a4_)
            {
                BOOST_TEST_EQ(a1, 42);
                BOOST_TEST_EQ(a2, 42.42f);
                BOOST_TEST_EQ(a3, "42");
                BOOST_TEST_EQ(&a4, &a4_);
                ++count;
            });
        BOOST_TEST_EQ(subscribe_count, 1);
        BOOST_TEST_EQ(aura(&s1).emit<event2_a4>(42, 42.42f, "42", std::ref(a4)), 0);
        BOOST_TEST_EQ(count, 0);
        BOOST_TEST_EQ(aura(&s1).emit<event1_a4>(42, 42.42f, "42", std::ref(a4)), 1);
        BOOST_TEST_EQ(count, 1);
        c1.reset();
        BOOST_TEST_EQ(subscribe_count, 0);
        {
            std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<event1_a4>(false);
            BOOST_TEST(!std::weak_ptr<void>().owner_before(tled->sl_) && !tled->sl_.owner_before(std::weak_ptr<void>()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
}

int main(int argc, char const * argv[])
{
    test_a0();
    test_a1();
    test_a2();
    test_a3();
    test_a4();
    return boost::report_errors();
}
