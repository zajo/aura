// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/thread_local_queue.hpp>
#include <boost/aura/subscribe.hpp>
#include <boost/aura/subscribe.hpp>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <algorithm>
#include <condition_variable>

namespace boost { namespace aurae {

namespace detail
{
    int emit_from_source(thread_local_event_data::subscription_list &, void const *, args_binder_base const *);

    class posted_events_wait_state
    {
        posted_events_wait_state(posted_events_wait_state const &);
        posted_events_wait_state & operator=(posted_events_wait_state const &);

    public:

        std::mutex mut_;
        std::condition_variable cond_;
        unsigned pending_count_;

        posted_events_wait_state():
            pending_count_(0)
        {
        }
    };

    class thread_local_event_data::posted_events
    {
        posted_events(posted_events const &);
        posted_events & operator=(posted_events const &);

        struct posted
        {
            unsigned serial_number;
            void const * s;
            std::shared_ptr<args_binder_base> args;
            posted()
            {
            }
            posted(unsigned serial_number, void const * s, std::shared_ptr<args_binder_base> const & args):
                serial_number(serial_number),
                s(s),
                args(args)
            {
                BOOST_AURA_ASSERT(s != 0);
            }
        };

        std::atomic<unsigned> & emit_serial_number_;
        std::thread::id const thread_id_;
        std::shared_ptr<posted_events_wait_state> const wait_state_;
        std::deque<posted> q_;

    public:

        posted_events(std::atomic<unsigned> & emit_serial_number, std::shared_ptr<posted_events_wait_state> const & wait_state):
            emit_serial_number_(emit_serial_number),
            thread_id_(std::this_thread::get_id()),
            wait_state_(wait_state)
        {
            BOOST_AURA_ASSERT(wait_state_);
        }

        bool post(void const * s, args_binder_base const * args)
        {
            BOOST_AURA_ASSERT(s != 0);
            if (thread_id_ == std::this_thread::get_id())
                return false;
            else
            {
                std::shared_ptr<args_binder_base> a;
                if (args)
                    a = args->clone();
                {
                    std::lock_guard<std::mutex> lk(wait_state_->mut_);
                    q_.push_back(posted(emit_serial_number_++, s, a));
                    ++wait_state_->pending_count_;
                }
                wait_state_->cond_.notify_one();
                return true;
            }
        }

        int emit_if_serial_number_matches(unsigned serial_number, thread_local_event_data const & tled)
        {
            posted p;
            {
                std::lock_guard<std::mutex> lk(wait_state_->mut_);
                if (q_.empty())
                    return -1;
                if (q_.front().serial_number != serial_number)
                    return -1;
                p = q_.front();
                q_.pop_front();
                BOOST_AURA_ASSERT(wait_state_->pending_count_ != 0);
                --wait_state_->pending_count_;
            }
            BOOST_AURA_ASSERT(p.s != 0);
            if (is_muted_(tled, p.s))
                return 0;
            int n = 0;
            if (std::shared_ptr<thread_local_event_data::subscription_list> sl = tled.sl_.lock())
                n += emit_from_source(*sl, p.s, p.args.get());
            if (std::shared_ptr<thread_local_event_data::subscription_list> al = tled.al_.lock())
                n += emit_from_source(*al, p.s, p.args.get());
            return n;
        }
    };

    namespace
    {
        class tled_rec
        {
            std::weak_ptr<thread_local_event_data> tled_;
        public:

            explicit tled_rec(std::shared_ptr<thread_local_event_data> const & tled):
                tled_(tled)
            {
            }

            bool expired() const
            {
                return tled_.expired();
            }

            std::shared_ptr<thread_local_event_data> lock() const
            {
                return tled_.lock();
            }
        };

        template <class Container>
        static void purge(Container & c)
        {
            c.erase(std::remove_if(c.begin(), c.end(), [](tled_rec const & r) { return r.expired(); }), c.end());
        }
    }

    class subscription_list_list
    {
        subscription_list_list(subscription_list_list const &);
        subscription_list_list & operator=(subscription_list_list const &);
        std::vector<tled_rec> same_event_different_threads_;

    public:

        std::mutex mut_;

        subscription_list_list()
        {
        }

        void notify_subscription_list_created(std::shared_ptr<thread_local_event_data> const & tled)
        {
            std::lock_guard<std::mutex> lk(mut_);
            same_event_different_threads_.push_back(tled_rec(tled));
            purge(same_event_different_threads_);
        }

        int cross_thread_emit(void const * s, args_binder_base const * args)
        {
            BOOST_AURA_ASSERT(s != 0);
            int count = 0;
            std::lock_guard<std::mutex> lk(mut_);
            for (auto & r : same_event_different_threads_)
                if (std::shared_ptr<thread_local_event_data> sp = r.lock())
                    if (sp->ps_)
                        count += int(sp->ps_->post(s, args));
            return count;
        }
    };

    namespace
    {
        std::shared_ptr<subscription_list_list> create_subscription_list_list()
        {
            return std::make_shared<subscription_list_list>();
        }

        class thread_local_subscription_list_list
        {
            thread_local_subscription_list_list(thread_local_subscription_list_list const &);
            thread_local_subscription_list_list & operator=(thread_local_subscription_list_list const &);

            bool has_tlq_;
            std::atomic<unsigned> emit_serial_number_;
            unsigned last_poll_serial_number_;
            std::vector<tled_rec> same_thread_different_events_;
            std::shared_ptr<posted_events_wait_state> const wait_state_;

        public:

            thread_local_subscription_list_list():
                has_tlq_(false),
                emit_serial_number_(0),
                last_poll_serial_number_(emit_serial_number_),
                wait_state_(std::make_shared<posted_events_wait_state>())
            {
            }

            void notify_subscription_list_created(std::shared_ptr<thread_local_event_data> const & tled)
            {
                if (has_tlq_)
                {
                    std::shared_ptr<thread_local_event_data::posted_events> ps = std::make_shared<thread_local_event_data::posted_events>(emit_serial_number_, wait_state_);
                    {
                        std::lock_guard<std::mutex> lk(tled->get_sll_(&create_subscription_list_list)->mut_);
                        tled->ps_.swap(ps);
                    }
                }
                same_thread_different_events_.push_back(tled_rec(tled));
                purge(same_thread_different_events_);
            }

            void enable_tlq()
            {
                BOOST_AURA_ASSERT(!has_tlq_);
                for (auto & r : same_thread_different_events_)
                    if (std::shared_ptr<thread_local_event_data> sp = r.lock())
                    {
                        std::shared_ptr<thread_local_event_data::posted_events> ps = std::make_shared<thread_local_event_data::posted_events>(emit_serial_number_, wait_state_);
                        {
                            std::lock_guard<std::mutex> lk(sp->get_sll_(&create_subscription_list_list)->mut_);
                            sp->ps_.swap(ps);
                        }
                    }
                has_tlq_ = true;
            }

            void disable_tlq()
            {
                BOOST_AURA_ASSERT(has_tlq_);
                for (auto & r : same_thread_different_events_)
                    if (std::shared_ptr<thread_local_event_data> sp = r.lock())
                    {
                        std::shared_ptr<thread_local_event_data::posted_events> ps;
                        {
                            std::lock_guard<std::mutex> lk(sp->get_sll_(&create_subscription_list_list)->mut_);
                            sp->ps_.swap(ps);
                        }
                    }
                has_tlq_ = false;
                last_poll_serial_number_ = emit_serial_number_;
                {
                    std::lock_guard<std::mutex> lk(wait_state_->mut_);
                    wait_state_->pending_count_ = 0;
                }
            }

            int poll()
            {
                unsigned current = emit_serial_number_;
                unsigned last_poll_serial_number = last_poll_serial_number_;
                last_poll_serial_number_ = current;
                int count = 0;
                for (unsigned serial_number = last_poll_serial_number; serial_number != current; ++serial_number)
                {
                    bool found = std::find_if(same_thread_different_events_.begin(), same_thread_different_events_.end(),
                        [&count, serial_number](tled_rec const & r)
                        {
                            if (std::shared_ptr<thread_local_event_data> sp = r.lock())
                            {
                                int n = sp->ps_->emit_if_serial_number_matches(serial_number, *sp);
                                if (n >= 0)
                                {
                                    count += n;
                                    return true;
                                }
                                else
                                    BOOST_AURA_ASSERT(n == -1);
                            }
                            return false;
                        }) != same_thread_different_events_.end();
                    BOOST_AURA_ASSERT(found);
                }
                return count;
            }

            int wait()
            {
                for (; ;)
                {
                    {
                        std::unique_lock<std::mutex> lk(wait_state_->mut_);
                        wait_state_->cond_.wait(lk, [this] { return wait_state_->pending_count_ != 0; });
                    }
                    if (int n = poll())
                        return n;
                }
            }
        };

        std::shared_ptr<thread_local_subscription_list_list> get_thread_local_subscription_list_list()
        {
            static thread_local std::shared_ptr<thread_local_subscription_list_list> tlsll(std::make_shared<thread_local_subscription_list_list>());
            return tlsll;
        }
    }

    cross_thread_interface * get_cross_thread_api()
    {
        class cross_thread_impl:
            public cross_thread_interface
        {
            void notify_subscription_list_created(std::shared_ptr<thread_local_event_data> const & tled) final override
            {
                tled->get_sll_(&create_subscription_list_list)->notify_subscription_list_created(tled);
                get_thread_local_subscription_list_list()->notify_subscription_list_created(tled);
            }

            int emit(thread_local_event_data const & tled, void const * s, args_binder_base const * args) final override
            {
                return tled.get_sll_(&create_subscription_list_list)->cross_thread_emit(s, args);
            }
        };
        static cross_thread_impl impl;
        return &impl;
    }
}

namespace
{
    struct bare_lambda: event<void(std::function<void()> const &), thread_affinity::cross_thread> {};
}

thread_local_queue::thread_local_queue()
{
}

thread_local_queue::~thread_local_queue()
{
}

namespace detail
{
    namespace
    {
        class thread_local_queue_impl:
            public thread_local_queue
        {
            thread_local_queue_impl(thread_local_queue_impl const &);
            thread_local_queue_impl & operator=(thread_local_queue_impl const &);

            std::thread::id const tid_;
            std::shared_ptr<thread_local_subscription_list_list> const tlsll_;

        public:

            thread_local_queue_impl():
                tid_(std::this_thread::get_id()),
                tlsll_(get_thread_local_subscription_list_list())
            {
                tlsll_->enable_tlq();
            }

            ~thread_local_queue_impl()
            {
                tlsll_->disable_tlq();
            }

            int poll() final override
            {
                BOOST_AURA_ASSERT(tid_ == std::this_thread::get_id());
                return tlsll_->poll();
            }

            int wait() final override
            {
                BOOST_AURA_ASSERT(tid_ == std::this_thread::get_id());
                return tlsll_->wait();
            }

            void post(std::function<void()> const & f) final override
            {
                BOOST_AURA_ASSERT(f);
                aura(static_cast<thread_local_queue *>(this)).emit<bare_lambda>(f);
            }
        };
    }
}

std::shared_ptr<thread_local_queue> create_thread_local_queue()
{
    std::shared_ptr<thread_local_queue> tlq = std::make_shared<detail::thread_local_queue_impl>();
    (void) persist(aura(tlq).subscribe<bare_lambda>([](std::function<void()> const & f)
        {
            BOOST_AURA_ASSERT(f);
            f();
        }));
    return tlq;
}

} }
