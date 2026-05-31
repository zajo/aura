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
    std::atomic<int> subscription_count;
    int source;
    struct event1: aura::event<void(), aura::thread_affinity::cross_thread> {};
    struct event2: aura::event<void(), aura::thread_affinity::cross_thread> {};
    struct terminate_thread: aura::event<void(), aura::thread_affinity::cross_thread> {};

    class thread_subscription_counter
    {
        thread_subscription_counter(thread_subscription_counter const &);
        thread_subscription_counter & operator=(thread_subscription_counter const &);

    public:

        thread_subscription_counter()
        {
            ++subscription_count;
        }

        ~thread_subscription_counter()
        {
            --subscription_count;
        }
    };

    void emitting_thread(barrier & b, std::weak_ptr<void> const & terminate)
    {
        std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
        bool keep_going = true;
        auto c = aura(terminate).subscribe<terminate_thread>(
            [&keep_going]
            {
                keep_going = false;
            });
        c->set_user_data(std::make_shared<thread_subscription_counter>());
        b.wait();
        while (keep_going)
        {
            (void) aura(&source).emit<event1>();
            (void) aura(&source).emit<event2>();
            (void) tlq->poll();
        }
    }

    void test(int emitting_thread_count, int per_thread_emit_count, int series_count)
    {
        assert(emitting_thread_count > 0);
        assert(per_thread_emit_count > 0);
        assert(series_count > 0);
        std::cout << "*** " << emitting_thread_count << '/' << per_thread_emit_count << '/' << series_count << " ***" << std::endl;
        barrier b(emitting_thread_count + 1);
        std::shared_ptr<int> terminate(std::make_shared<int>(42));
        std::vector<std::thread> tgr;
        tgr.reserve(emitting_thread_count);
        for (int i = 0; i != emitting_thread_count; ++i)
            tgr.emplace_back(
                [&b, &terminate]
                {
                    emitting_thread(b, terminate);
                });
        b.wait();
        BOOST_TEST(terminate.use_count() == 1);
        BOOST_TEST_EQ(subscription_count, emitting_thread_count);
        std::shared_ptr<aura::subscription> c1 = aura(&source).subscribe<event1>([](){});
        std::shared_ptr<aura::subscription> c2 = aura(&source).subscribe<event2>([](){});
        for (int i = 0; i != series_count; ++i)
        {
            std::shared_ptr<aurae::thread_local_queue> tlq = aurae::create_thread_local_queue();
            for (int j = 0; j < emitting_thread_count * per_thread_emit_count; )
            {
                int n = tlq->poll();
                j += n;
            }
        }
        std::cout << "Requesting terminate..." << std::endl;
        int n = aura(terminate.get()).emit<terminate_thread>();
        BOOST_TEST_EQ(n, emitting_thread_count);
        std::cout << "Joining..." << std::endl;
        for (auto & t : tgr)
            t.join();
        std::cout << "Joined." << std::endl;
        BOOST_TEST_EQ(subscription_count, 0);
    }
}

int main(int argc, char const * argv[])
{
    test(1, 1, 50);
    test(15, 5, 50);
    test(25, 10, 50);
    return boost::report_errors();
}
