// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/attach.hpp>
#include <boost/aura/thread_local_queue.hpp>
#include "barrier.hpp"
#include <thread>
#include <string>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    int source;

    struct attr1: aura::attribute<std::string, aura::thread_affinity::cross_thread> {};
}

int main(int, char const *[])
{
    {
        barrier b1(2);
        barrier b2(2);
        barrier b3(2);
        barrier b4(2);

        std::thread setter([&]
            {
                std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
                auto a = aura(&source).attach<attr1>("setter-init");
                BOOST_TEST(a);
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "setter-init");
                b1.wait();
                int n = aura(&source).set<attr1>("from-setter");
                BOOST_TEST_EQ(n, 2);
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "from-setter");
                b2.wait();
                b3.wait();
                while (tlq->poll())
                    { }
                b4.wait();
            });

        std::thread getter([&]
            {
                std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
                auto a = aura(&source).attach<attr1>("getter-init");
                BOOST_TEST(a);
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "getter-init");
                b1.wait();
                b2.wait();
                while (aura(&source).get<attr1>() != "from-setter")
                    tlq->poll();
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "from-setter");
                b3.wait();
                b4.wait();
            });

        setter.join();
        getter.join();
    }

    {
        barrier b1(2);
        barrier b2(2);
        barrier b3(2);

        std::thread t_late([&]
            {
                std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
                b1.wait();
                b2.wait();
                auto a = aura(&source).attach<attr1>("late-init");
                BOOST_TEST(a);
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "late-init");
                while (tlq->poll())
                    { }
                BOOST_TEST_EQ(aura(&source).get<attr1>(), "late-init");
                b3.wait();
            });

        std::thread t_early([&]
            {
                std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
                auto a = aura(&source).attach<attr1>("early-init");
                BOOST_TEST(a);
                b1.wait();
                int n = aura(&source).set<attr1>("early-set");
                BOOST_TEST_EQ(n, 1);
                b2.wait();
                b3.wait();
                while (tlq->poll())
                    { }
            });

        t_late.join();
        t_early.join();
    }

    return boost::report_errors();
}
