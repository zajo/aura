// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include <functional>
#include "boost/core/lightweight_test.hpp"

using aura = boost::aurae::aura;
using namespace std::placeholders;
struct test_event1: aura::event<void()> {};
struct test_event2: aura::event<void()> {};

namespace
{
    void test_fn()
    {
        int source;
        int count1 = 0;
        int count2 = 0;
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        std::shared_ptr<aura::subscription const> c1 = aura(&source).subscribe<test_event1>([&count1](){ ++count1; });
        BOOST_TEST(c1.use_count() == 1);
        std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<test_event2>([&count2](){ ++count2; });
        BOOST_TEST(c2.use_count() == 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        c1.reset();
        c2.reset();
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
    }

    void test_target_fn()
    {
        int source;
        int target;
        int count1 = 0;
        int count2 = 0;
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        std::shared_ptr<aura::subscription const> c1 = aura(&source).subscribe<test_event1>(&target, [&count1, &target](int * t){ BOOST_TEST(t == &target); ++count1; });
        BOOST_TEST(c1.use_count() == 1);
        std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<test_event2>(&target, [&count2, &target](int * t){ BOOST_TEST(t == &target); ++count2; });
        BOOST_TEST(c2.use_count() == 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        c1.reset();
        c2.reset();
        BOOST_TEST(aura(&source).emit<test_event1>() == 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
    }

    struct test_class
    {
        int & count;
        explicit
        test_class(int & count):
            count(count)
        {
        }
        void
        fn()
        {
            ++count;
        }
    };

    void test_target_mem_fn()
    {
        int source;
        int count1 = 0;
        int count2 = 0;
        test_class target1(count1);
        test_class target2(count2);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        std::shared_ptr<aura::subscription const> c1 = aura(&source).subscribe<test_event1>(&target1, &test_class::fn);
        BOOST_TEST(c1.use_count() == 1);
        std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<test_event2>(&target2, &test_class::fn);
        BOOST_TEST(c2.use_count() == 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        c1.reset();
        c2.reset();
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
    }

    void test_target_mem_fn_bind()
    {
        int source;
        int count1 = 0;
        int count2 = 0;
        test_class target1(count1);
        test_class target2(count2);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        std::shared_ptr<aura::subscription const> c1 = aura(&source).subscribe<test_event1>(&target1, std::bind(&test_class::fn, _1));
        BOOST_TEST(c1.use_count() == 1);
        std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<test_event2>(&target2, std::bind(&test_class::fn, _1));
        BOOST_TEST(c2.use_count() == 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 0);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 1);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 1);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 1);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        c1.reset();
        c2.reset();
        BOOST_TEST_EQ(aura(&source).emit<test_event1>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
        BOOST_TEST_EQ(aura(&source).emit<test_event2>(), 0);
        BOOST_TEST_EQ(count1, 2);
        BOOST_TEST_EQ(count2, 2);
    }
}

int main(int argc, char const * argv[])
{
    test_fn();
    test_target_fn();
    test_target_mem_fn();
    test_target_mem_fn_bind();
    return boost::report_errors();
}
