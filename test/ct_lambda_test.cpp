// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/thread_local_queue.hpp>
#include <boost/aura/subscribe.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    int const thread_count = 10;
    int const iteration_count = 1000;

    void emitting_thread(std::weak_ptr<aurae::thread_local_queue> tlq, std::atomic<bool> & stop, int & counter, std::thread::id tid)
    {
        int post_count = 0;
        while (!stop && post_count != iteration_count * 3)
            if (std::shared_ptr<aurae::thread_local_queue> p = tlq.lock())
            {
                p->post([&counter, tid]()
                    {
                        BOOST_TEST_EQ(std::this_thread::get_id(), tid);
                        ++counter;
                    });
                ++post_count;
            }
            else
                break;
    }

    void consuming_thread()
    {
        assert(iteration_count > 0);
        int count = 0;
        std::atomic<bool> stop(false);
        std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
        std::thread::id const tid = std::this_thread::get_id();
        std::thread th([&]
            {
                emitting_thread(tlq, stop, count, tid);
            });
        while (count < iteration_count)
            tlq->poll();
        stop = true;
        th.join();
        while (tlq->poll())
            { }
        tlq.reset();
    }
}

int main(int argc, char const * argv[])
{
    std::vector<std::thread> tgr;
    tgr.reserve(thread_count);
    for (int i = 0; i != thread_count; ++i)
        tgr.emplace_back(&consuming_thread);
    for (auto & t : tgr)
        t.join();
    return boost::report_errors();
}
