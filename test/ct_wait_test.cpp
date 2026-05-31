// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/thread_local_queue.hpp>
#include <boost/aura/subscribe.hpp>
#include <thread>
#include <chrono>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    int const iteration_count = 50;
    struct sig1: aura::event<void(), aura::thread_affinity::cross_thread> {};
    struct sig2: aura::event<void(), aura::thread_affinity::cross_thread> {};

    template <class Event>
    void emitting_thread(int & counter)
    {
        for (int i = 0; i != iteration_count; ++i)
        {
            aura(&counter).emit<Event>();
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100));
        }
    }

    void test()
    {
        std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
        int counter1 = 0; std::shared_ptr<aura::subscription const> c1 = aura(&counter1).subscribe<sig1>([&counter1](){ ++counter1; });
        int counter2 = 0; std::shared_ptr<aura::subscription const> c2 = aura(&counter2).subscribe<sig2>([&counter2](){ ++counter2; });
        std::thread th1(
            [&]
            {
                emitting_thread<sig1>(counter1);
            });
        std::thread th2(
            [&]
            {
                emitting_thread<sig2>(counter2);
            });
        while (counter1 != iteration_count || counter2 != iteration_count)
        {
            int n = tlq->wait();
            BOOST_TEST_GT(n, 0);
            BOOST_TEST_GE(counter1, 0);
            BOOST_TEST_LE(counter1, iteration_count);
            BOOST_TEST_GE(counter2, 0);
            BOOST_TEST_LE(counter2, iteration_count);
        }
        th1.join();
        th2.join();
        tlq.reset();
        BOOST_TEST_EQ(counter1, iteration_count);
        BOOST_TEST_EQ(counter2, iteration_count);
    }
}

int main(int argc, char const * argv[])
{
    test();
    return boost::report_errors();
}
