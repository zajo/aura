// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/thread_local_queue.hpp>
#include <boost/aura/subscribe.hpp>
#include "barrier.hpp"
#include <thread>
#include <vector>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    int source;
    struct event1: aura::event<void(), aura::thread_affinity::cross_thread> {};
    struct event2: aura::event<void(), aura::thread_affinity::cross_thread> {};

    void emitting_thread(int consuming_thread_count, int iterations)
    {
        int local_count1 = 0; std::shared_ptr<aura::subscription> c1 = aura(&source).subscribe<event1>([&local_count1]() { ++local_count1; });
        int local_count2 = 0; std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<event2>([&local_count2]() { ++local_count2; });
        for (int i = 0; i != iterations; ++i)
        {
            int n1 = aura(&source).emit<event1>();
            BOOST_TEST_EQ(local_count1, i + 1);
            BOOST_TEST_EQ(n1, consuming_thread_count + 1);
            int n2 = aura(&source).emit<event2>();
            BOOST_TEST_EQ(local_count2, i + 1);
            BOOST_TEST_EQ(n2, consuming_thread_count + 1);
        }
    }

    void consuming_thread(barrier & b, int total_count)
    {
        assert(total_count > 0);
        int n1 = 0; std::shared_ptr<aura::subscription> c1 = aura(&source).subscribe<event1>([&n1]() { ++n1; });
        std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
        int n2 = 0; std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<event2>([&n2]() { ++n2; });
        b.wait();
        while (n1 != total_count || n2 != total_count)
        {
            BOOST_TEST_GE(n1, 0);
            BOOST_TEST_LE(n1, total_count);
            BOOST_TEST_GE(n2, 0);
            BOOST_TEST_LE(n2, total_count);
            int n1svd = n1, n2svd = n2;
            int n = tlq->poll();
            BOOST_TEST_EQ(n, (n1 - n1svd) + (n2 - n2svd));
        }
    }

    void test(int emitting_thread_count, int consuming_thread_count, int iterations)
    {
        assert(emitting_thread_count > 0);
        assert(consuming_thread_count > 0);
        assert(iterations > 0);
        barrier b(consuming_thread_count + 1);
        std::vector<std::thread> tgr;
        tgr.reserve(emitting_thread_count + consuming_thread_count);
        int const total_count = emitting_thread_count * iterations;
        for (int i = 0; i != consuming_thread_count; ++i)
            tgr.emplace_back([total_count, &b]() { consuming_thread(b, total_count); });
        b.wait();
        for (int i = 0; i != emitting_thread_count; ++i)
            tgr.emplace_back([consuming_thread_count, iterations]() { emitting_thread(consuming_thread_count, iterations); });
        for (auto & t : tgr)
            t.join();
    }
}

int main(int argc, char const * argv[])
{
    test(1, 1, 100);

    test(10, 1, 100);
    test(1, 10, 100);
    test(10, 10, 100);

    test(50, 1, 100);
    test(1, 50, 100);
    test(10, 50, 100);

    return boost::report_errors();
}
