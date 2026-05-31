// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/mute.hpp>
#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"

namespace aurae = boost::aurae;
using aura = aurae::aura;

namespace
{
    struct my_source_type {};

    struct meta_data
    {
        std::shared_ptr<my_source_type> s;
        my_source_type * sp;
        aura::muting * sb;
        int count;

        meta_data():
            sp(0),
            sb(0),
            count(0)
        {
        }

        void reset()
        {
            s.reset();
            sp = 0;
            sb = 0;
        }
    };

    bool does_not_own(std::shared_ptr<void const> const & s)
    {
        return !(s.owner_before(std::shared_ptr<void const>())) && !(std::shared_ptr<void const>().owner_before(s));
    }

    template <class Event>
    bool source_muted(void const * s)
    {
        std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<Event>(false);
        return tled && aurae::detail::is_muted_(*tled, s);
    }

    template <class Event>
    std::weak_ptr<aurae::detail::thread_local_event_data::subscription_list> const & muted_sl()
    {
        static std::weak_ptr<aurae::detail::thread_local_event_data::subscription_list> empty;
        std::shared_ptr<aurae::detail::thread_local_event_data> const & tled = aurae::detail::get_thread_local_event_data<Event>(false);
        return tled ? tled->ml_ : empty;
    }

    template <class Event>
    meta_data & the_meta_data()
    {
        static meta_data d;
        return d;
    }
    struct event1: aura::event<void()> {};
    struct event2: aura::event<void()> {};

    template <class Event>
    void on_muted(aurae::muting & eb, unsigned context)
    {
        meta_data & d = the_meta_data<Event>();
        BOOST_TEST(!d.s);
        BOOST_TEST(!d.sp);
        BOOST_TEST(!d.sb);
        d.s = eb.source<my_source_type>();
        d.sp = eb.source<my_source_type>().get();
        d.sb = &eb;
        d.count += (context & aura::meta::context_flags::subscribing) ? 1 : -1;
    }

    void mute_test()
    {
        {
            my_source_type s1;
            my_source_type s2;
            BOOST_TEST(!source_muted<event1>(&s1));
            BOOST_TEST(!source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(!source_muted<event2>(&s2));
            std::shared_ptr<aura::muting> eb1 = aura(&s1).mute<event1>();
            {
                std::shared_ptr<aura::muting> b1 = aura(&s1).mute<event1>();
                std::shared_ptr<aura::muting> b2 = aura(&s1).mute<event1>();
                BOOST_TEST_EQ(eb1, b1);
                BOOST_TEST_EQ(eb1, b2);
            }
            BOOST_TEST(does_not_own(the_meta_data<event1>().s));
            BOOST_TEST_EQ(the_meta_data<event1>().sp, &s1);
            BOOST_TEST_EQ(the_meta_data<event1>().sb, eb1.get());
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            the_meta_data<event1>().reset();
            BOOST_TEST(source_muted<event1>(&s1));
            BOOST_TEST(!source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(!source_muted<event2>(&s2));
            std::shared_ptr<aura::muting> eb2 = aura(&s1).mute<event2>();
            {
                std::shared_ptr<aura::muting> b1 = aura(&s1).mute<event2>();
                std::shared_ptr<aura::muting> b2 = aura(&s1).mute<event2>();
                BOOST_TEST_EQ(eb2, b1);
                BOOST_TEST_EQ(eb2, b2);
            }
            BOOST_TEST(does_not_own(the_meta_data<event2>().s));
            BOOST_TEST_EQ(the_meta_data<event2>().sp, &s1);
            BOOST_TEST_EQ(the_meta_data<event2>().sb, eb2.get());
            BOOST_TEST_EQ(the_meta_data<event2>().count, 1);
            the_meta_data<event2>().reset();
            BOOST_TEST(source_muted<event1>(&s1));
            BOOST_TEST(source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(!source_muted<event2>(&s2));
            std::shared_ptr<aura::muting> eb3 = aura(&s2).mute<event1>();
            {
                std::shared_ptr<aura::muting> b1 = aura(&s2).mute<event1>();
                std::shared_ptr<aura::muting> b2 = aura(&s2).mute<event1>();
                BOOST_TEST_EQ(eb3, b1);
                BOOST_TEST_EQ(eb3, b2);
            }
            BOOST_TEST(does_not_own(the_meta_data<event1>().s));
            BOOST_TEST_EQ(the_meta_data<event1>().sp, &s2);
            BOOST_TEST_EQ(the_meta_data<event1>().sb, eb3.get());
            BOOST_TEST_EQ(the_meta_data<event1>().count, 2);
            the_meta_data<event1>().reset();
            BOOST_TEST(source_muted<event1>(&s1));
            BOOST_TEST(source_muted<event2>(&s1));
            BOOST_TEST(source_muted<event1>(&s2));
            BOOST_TEST(!source_muted<event2>(&s2));
            std::shared_ptr<aura::muting> eb4 = aura(&s2).mute<event2>();
            {
                std::shared_ptr<aura::muting> b1 = aura(&s2).mute<event2>();
                std::shared_ptr<aura::muting> b2 = aura(&s2).mute<event2>();
                BOOST_TEST_EQ(eb4, b1);
                BOOST_TEST_EQ(eb4, b2);
            }
            BOOST_TEST(does_not_own(the_meta_data<event2>().s));
            BOOST_TEST_EQ(the_meta_data<event2>().sp, &s2);
            BOOST_TEST_EQ(the_meta_data<event2>().sb, eb4.get());
            BOOST_TEST_EQ(the_meta_data<event2>().count, 2);
            the_meta_data<event2>().reset();
            BOOST_TEST(source_muted<event1>(&s1));
            BOOST_TEST(source_muted<event2>(&s1));
            BOOST_TEST(source_muted<event1>(&s2));
            BOOST_TEST(source_muted<event2>(&s2));
            {
                aura::muting * eb = eb1.get();
                eb1.reset();
                BOOST_TEST(!muted_sl<event1>().expired());
                BOOST_TEST(does_not_own(the_meta_data<event1>().s));
                BOOST_TEST_EQ(the_meta_data<event1>().sp, &s1);
                BOOST_TEST_EQ(the_meta_data<event1>().sb, eb);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            the_meta_data<event1>().reset();
            BOOST_TEST(!source_muted<event1>(&s1));
            BOOST_TEST(source_muted<event2>(&s1));
            BOOST_TEST(source_muted<event1>(&s2));
            BOOST_TEST(source_muted<event2>(&s2));
            {
                aura::muting * eb = eb3.get();
                eb3.reset();
                BOOST_TEST(muted_sl<event1>().expired());
                BOOST_TEST(does_not_own(the_meta_data<event1>().s));
                BOOST_TEST_EQ(the_meta_data<event1>().sp, &s2);
                BOOST_TEST_EQ(the_meta_data<event1>().sb, eb);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().count, 0);
            the_meta_data<event1>().reset();
            BOOST_TEST(!source_muted<event1>(&s1));
            BOOST_TEST(source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(source_muted<event2>(&s2));
            {
                aura::muting * eb = eb2.get();
                eb2.reset();
                BOOST_TEST(!muted_sl<event2>().expired());
                BOOST_TEST(does_not_own(the_meta_data<event2>().s));
                BOOST_TEST_EQ(the_meta_data<event2>().sp, &s1);
                BOOST_TEST_EQ(the_meta_data<event2>().sb, eb);
            }
            BOOST_TEST_EQ(the_meta_data<event2>().count, 1);
            the_meta_data<event2>().reset();
            BOOST_TEST(!source_muted<event1>(&s1));
            BOOST_TEST(!source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(source_muted<event2>(&s2));
            {
                aura::muting * eb = eb4.get();
                eb4.reset();
                BOOST_TEST(muted_sl<event2>().expired());
                BOOST_TEST(does_not_own(the_meta_data<event2>().s));
                BOOST_TEST_EQ(the_meta_data<event2>().sp, &s2);
                BOOST_TEST_EQ(the_meta_data<event2>().sb, eb);
            }
            BOOST_TEST_EQ(the_meta_data<event2>().count, 0);
            the_meta_data<event2>().reset();
            BOOST_TEST(!source_muted<event1>(&s1));
            BOOST_TEST(!source_muted<event2>(&s1));
            BOOST_TEST(!source_muted<event1>(&s2));
            BOOST_TEST(!source_muted<event2>(&s2));
        }

        {
            std::shared_ptr<my_source_type> s(new my_source_type);
            BOOST_TEST(!source_muted<event1>(s.get()));
            BOOST_TEST(the_meta_data<event1>().count == 0);
            std::shared_ptr<aura::muting> eb = aura(s).mute<event1>();
            {
                std::shared_ptr<aura::muting> b1 = aura(s).mute<event1>();
                std::shared_ptr<aura::muting> b2 = aura(s).mute<event1>();
                BOOST_TEST_EQ(eb, b1);
                BOOST_TEST_EQ(eb, b2);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().s, s);
            BOOST_TEST_EQ(the_meta_data<event1>().sp, s.get());
            BOOST_TEST_EQ(the_meta_data<event1>().sb, eb.get());
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            BOOST_TEST(source_muted<event1>(s.get()));
            the_meta_data<event1>().reset();
            s.reset();
            BOOST_TEST_EQ(the_meta_data<event1>().sb, nullptr);
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            the_meta_data<event1>().reset();
            {
                aura::muting * p = eb.get();
                eb.reset();
                BOOST_TEST(!the_meta_data<event1>().s);
                BOOST_TEST(!the_meta_data<event1>().sp);
                BOOST_TEST_EQ(the_meta_data<event1>().sb, p);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().count, 0);
            the_meta_data<event1>().count = 0;
            the_meta_data<event1>().reset();
        }

        {
            std::shared_ptr<my_source_type> s(new my_source_type);
            BOOST_TEST(!source_muted<event1>(s.get()));
            BOOST_TEST_EQ(the_meta_data<event1>().count, 0);
            std::shared_ptr<aura::muting> eb = aura(std::weak_ptr<my_source_type>(s)).mute<event1>();
            {
                std::shared_ptr<aura::muting> b1 = aura(s).mute<event1>();
                std::shared_ptr<aura::muting> b2 = aura(s).mute<event1>();
                BOOST_TEST_EQ(eb, b1);
                BOOST_TEST_EQ(eb, b2);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().s, s);
            BOOST_TEST_EQ(the_meta_data<event1>().sp, s.get());
            BOOST_TEST_EQ(the_meta_data<event1>().sb, eb.get());
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            BOOST_TEST(source_muted<event1>(s.get()));
            the_meta_data<event1>().reset();
            s.reset();
            BOOST_TEST_EQ(the_meta_data<event1>().sb, nullptr);
            BOOST_TEST_EQ(the_meta_data<event1>().count, 1);
            the_meta_data<event1>().reset();
            {
                aura::muting * p = eb.get();
                eb.reset();
                BOOST_TEST(!the_meta_data<event1>().s);
                BOOST_TEST(!the_meta_data<event1>().sp);
                BOOST_TEST_EQ(the_meta_data<event1>().sb, p);
            }
            BOOST_TEST_EQ(the_meta_data<event1>().count, 0);
            the_meta_data<event1>().count = 0;
            the_meta_data<event1>().reset();
        }
    }

    struct null_deleter { template <class T> void operator()(T *) { } };

    void source_address_reuse_test()
    {
        my_source_type s2;
        std::shared_ptr<my_source_type> s1(&s2, null_deleter());
        std::shared_ptr<aura::muting> b1 = aura(s1).mute<event1>();
        BOOST_TEST(source_muted<event1>(&s2));
        BOOST_TEST_EQ(b1.use_count(), 1);
        s1.reset();
        the_meta_data<event1>().reset();
        std::shared_ptr<aura::muting> b2 = aura(&s2).mute<event1>();
        BOOST_TEST(source_muted<event1>(&s2));
        BOOST_TEST_EQ(b2.use_count(), 1);
        the_meta_data<event1>().reset();
        b1.reset();
        BOOST_TEST(source_muted<event1>(&s2));
        the_meta_data<event1>().reset();
        b2.reset();
        BOOST_TEST(!source_muted<event1>(&s2));
    }

}

int main(int argc, char const * argv[])
{
    auto m1 = aura::meta{}.subscribe<aura::meta::muted<event1>>(&on_muted<event1>);
    auto m2 = aura::meta{}.subscribe<aura::meta::muted<event2>>(&on_muted<event2>);
    mute_test();
    source_address_reuse_test();
    return boost::report_errors();
}
