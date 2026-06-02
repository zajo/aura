#ifndef BOOST_AURA_DETAIL_COMMON_HPP_INCLUDED
#define BOOST_AURA_DETAIL_COMMON_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/event.hpp>
#include <boost/aura/meta.hpp>
#include <boost/aura/detail/weak_store.hpp>
#include <memory>
#include <atomic>

namespace boost { namespace aurae {

namespace detail
{
    class subscription_list_list;
    class thread_local_event_data;
    class emit_args_binder;
    class subscription_base;

    std::shared_ptr<void const> & meta_source();
    using emit_meta_fn = int (*)(subscription_base &, unsigned);

    using emit_fn = int(thread_local_event_data const &, void const *, emit_args_binder const *);
    using cleanup_fn = void(thread_local_event_data const &);

    int emit_stub(thread_local_event_data const &, void const *, emit_args_binder const *);
    void cleanup_stub(thread_local_event_data const &);

    ////////////////////////////////////////

    class cross_thread_interface
    {
    protected:

        constexpr cross_thread_interface() noexcept { }
        ~cross_thread_interface() { }

    public:

        virtual void notify_subscription_list_created(std::shared_ptr<thread_local_event_data> const &) = 0;
        virtual int emit(thread_local_event_data const &, void const *, emit_args_binder const *) = 0;
    };

    ////////////////////////////////////////

    class thread_local_event_data
    {
        thread_local_event_data(thread_local_event_data const &);
        thread_local_event_data & operator=(thread_local_event_data const &);
        cleanup_fn * cleanup_;
        std::atomic<int> * const sl_count_;

    public:

        emit_fn * emit_;

        class subscription_list;
        friend class subscription_list;
        std::weak_ptr<subscription_list> sl_;
        std::weak_ptr<subscription_list> ml_;
        std::weak_ptr<subscription_list> al_;

        class posted_events;
        std::shared_ptr<posted_events> ps_;

        std::shared_ptr<thread_local_event_data const> keep_meta_subscribed_tled_afloat_;
        std::shared_ptr<thread_local_event_data const> keep_meta_attached_tled_afloat_;
        std::shared_ptr<thread_local_event_data const> keep_meta_muted_tled_afloat_;

        std::shared_ptr<subscription_list_list> const & (* const get_sll_)(std::shared_ptr<subscription_list_list> (*)());
        std::atomic<cross_thread_interface *> * const cross_thread_;
        bool cross_thread_registered_;

        thread_local_event_data():
            cleanup_(&cleanup_stub),
            sl_count_(0),
            emit_(&emit_stub),
            get_sll_(0),
            cross_thread_(0),
            cross_thread_registered_(false)
        {
        }

        thread_local_event_data(std::shared_ptr<subscription_list_list> const & (*get_sll)(std::shared_ptr<subscription_list_list> (*)()), std::atomic<int> & sl_count, std::atomic<cross_thread_interface *> & cross_thread):
            cleanup_(&cleanup_stub),
            sl_count_(&sl_count),
            emit_(&emit_stub),
            get_sll_(get_sll),
            cross_thread_(&cross_thread),
            cross_thread_registered_(false)
        {
        }

        ~thread_local_event_data()
        {
            cleanup_(*this);
        }

        static inline int emit_stub(thread_local_event_data const & tled, void const * s, emit_args_binder const * args)
        {
            if (tled.sl_count_ && *tled.sl_count_)
                if (cross_thread_interface * cross_thread = tled.cross_thread_->load())
                    return cross_thread->emit(tled, s, args);
            return 0;
        }

        static inline void cleanup_stub(thread_local_event_data const &)
        {
        }
    };

    typedef thread_local_event_data::subscription_list subscription_list;

    ////////////////////////////////////////

    class subscription_base
    {
        subscription_base(subscription_base const &);
        subscription_base & operator=(subscription_base const &);

        friend void register_(std::shared_ptr<subscription_base> const &, weak_store && s, weak_store && t, std::shared_ptr<void const> const & fn);
        friend std::weak_ptr<subscription_base const> persist(std::shared_ptr<subscription_base const> const &);
        friend std::shared_ptr<subscription_base const> release(std::weak_ptr<subscription_base const> const &);

    protected:

        std::shared_ptr<subscription_list> sl_;
        int position_;

        explicit subscription_base(std::shared_ptr<subscription_list> const & sl);
        ~subscription_base();

        weak_store const & source_() const;
        void unregister_();
    };

    std::weak_ptr<subscription_base const> persist(std::shared_ptr<subscription_base const> const &);
    std::shared_ptr<subscription_base const> release(std::weak_ptr<subscription_base const> const &);

    ////////////////////////////////////////

    template <class Event>
    std::shared_ptr<subscription_list_list> const & get_subscription_list_list(std::shared_ptr<subscription_list_list> (*create_subscription_list_list)())
    {
        static std::shared_ptr<subscription_list_list> obj(create_subscription_list_list());
        return obj;
    }

    template <class Event>
    std::shared_ptr<thread_local_event_data> const & get_thread_local_event_data(bool allocate);

    bool is_muted_(thread_local_event_data const &, void const *);

    template <class Event>
    struct register_with_non_meta
    {
        static void keep_afloat(std::shared_ptr<thread_local_event_data const> const &)
        {
        }
    };

    template <class Event>
    struct register_with_non_meta<meta::subscribed<Event>>
    {
        static void keep_afloat(std::shared_ptr<thread_local_event_data const> const & meta)
        {
            auto main_tled = get_thread_local_event_data<Event>(true);
            BOOST_AURA_ASSERT(!main_tled->keep_meta_subscribed_tled_afloat_);
            main_tled->keep_meta_subscribed_tled_afloat_ = meta;
        }
    };

    template <class Event>
    struct register_with_non_meta<meta::muted<Event>>
    {
        static void keep_afloat(std::shared_ptr<thread_local_event_data const> const & meta)
        {
            auto main_tled = get_thread_local_event_data<Event>(true);
            BOOST_AURA_ASSERT(!main_tled->keep_meta_muted_tled_afloat_);
            main_tled->keep_meta_muted_tled_afloat_ = meta;
        }
    };

    template <class Event, thread_affinity Affinity = Event::affinity>
    struct thread_local_event_data_;

    template <class Event>
    struct thread_local_event_data_<Event, thread_affinity::cross_thread>
    {
        static std::shared_ptr<thread_local_event_data> const & get(bool allocate)
        {
            static std::atomic<int> sl_count;
            static std::atomic<cross_thread_interface *> cross_thread;
            static thread_local std::shared_ptr<thread_local_event_data> obj;
            if (!obj && (allocate || cross_thread.load()))
            {
                obj = std::make_shared<thread_local_event_data>(&get_subscription_list_list<Event>, sl_count, cross_thread);
                register_with_non_meta<Event>::keep_afloat(obj);
            }
            return obj;
        }
    };

    template <class Event>
    struct thread_local_event_data_<Event, thread_affinity::emitting_thread>
    {
        static std::shared_ptr<thread_local_event_data> const & get(bool allocate)
        {
            static thread_local std::shared_ptr<thread_local_event_data> obj;
            if (!obj && allocate)
            {
                obj = std::make_shared<thread_local_event_data>();
                register_with_non_meta<Event>::keep_afloat(obj);
            }
            return obj;
        }
    };

    template <class Event>
    std::shared_ptr<thread_local_event_data> const & get_thread_local_event_data(bool allocate)
    {
        return thread_local_event_data_<Event>::get(allocate);
    }

    void register_(std::shared_ptr<subscription_base> const &, weak_store && s, weak_store && t, std::shared_ptr<void const> const & fn);
}

} }

#endif
