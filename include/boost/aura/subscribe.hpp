#ifndef BOOST_AURA_SUBSCRIBE_HPP_INCLUDED
#define BOOST_AURA_SUBSCRIBE_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/emit.hpp>
#include <utility>

namespace boost { namespace aurae {

class subscription:
    public detail::subscription_base
{
    subscription(subscription const &);
    subscription & operator=(subscription const &);

    template <class T>
    struct deleter_user_data
    {
        T value;

        explicit deleter_user_data(T value):
            value(value)
        {
        }

        void operator()(void const *) const
        {
        }
    };

    template <class T>
    typename std::enable_if<std::is_assignable<T &, T const &>::value>::type set_user_data_(T const & x)
    {
        if (deleter_user_data<T> * d = std::get_deleter<deleter_user_data<T>>(user_data_))
            d->value = x;
        else
            user_data_.reset((void *)0, deleter_user_data<T>(x));
    }

    template <class T>
    typename std::enable_if<!std::is_assignable<T &, T const &>::value>::type set_user_data_(T const & x)
    {
        user_data_.reset((void *)0, deleter_user_data<T>(x));
    }

    std::shared_ptr<void> user_data_;

    detail::weak_store const & target_() const;

public:

    explicit subscription(std::shared_ptr<detail::subscription_list> const & sl):
        detail::subscription_base(sl)
    {
    }

    ~subscription();

    template <class T>
    std::shared_ptr<T> source() const
    {
        return source_().maybe_lock<T>();
    }

    template <class T>
    std::shared_ptr<T> target() const
    {
        return target_().maybe_lock<T>();
    }

    template <class T>
    void set_user_data(T const & x)
    {
        set_user_data_(x);
    }

    template <class T>
    T * get_user_data() const
    {
        deleter_user_data<T> * d = std::get_deleter<deleter_user_data<T>>(user_data_);
        return d ? &d->value : 0;
    }
};

std::weak_ptr<subscription const> persist(std::shared_ptr<subscription const> const &);
std::weak_ptr<subscription> persist(std::shared_ptr<subscription> const &);

std::shared_ptr<subscription const> release(std::weak_ptr<subscription const> const &);
std::shared_ptr<subscription> release(std::weak_ptr<subscription> const &);

////////////////////////////////////////

namespace detail
{
    template <class Event>
    int emit_meta_subscribed(subscription_base & sb, unsigned context)
    {
        return emit_dispatch<meta::subscribed<Event>>(meta_source().get(), std::ref(static_cast<subscription &>(sb)), context);
    }

    std::shared_ptr<subscription> subscribe_(std::shared_ptr<thread_local_event_data> const &, weak_store && s, weak_store && t, std::shared_ptr<void const> const &, emit_meta_fn emit_meta);

    template <class Event, class F>
    std::shared_ptr<subscription> subscribe_fwd(weak_store && s, F f)
    {
        return subscribe_(
            get_thread_local_event_data<Event>(true),
            std::move(s),
            weak_store(),
            std::make_shared<std::function<typename Event::signature>>(f),
            &emit_meta_subscribed<Event>);
    }

    template <class Event, class Target, class F, class Signature>
    struct bind_front;

    template <class Event, class Target, class Rm, class... Am, class R, class... A>
    struct bind_front<Event, Target, Rm (Target::*)(Am...), R(A...)>
    {
        static std::shared_ptr<subscription> subscribe_fwd(weak_store && s, weak_store && t, Rm (Target::*f)(Am...))
        {
            return subscribe_(
                get_thread_local_event_data<Event>(true),
                std::move(s),
                std::move(t),
                std::make_shared<std::function<typename Event::signature>>([=](A... a) { return (t.maybe_lock<Target>().get()->*f)(a...); }),
                &emit_meta_subscribed<Event>);
        }
    };

    template <class Event, class Target, class F, class R, class... A>
    struct bind_front<Event, Target, F, R(A...)>
    {
        static std::shared_ptr<subscription> subscribe_fwd(weak_store && s, weak_store && t, F f)
        {
            return subscribe_(
                get_thread_local_event_data<Event>(true),
                std::move(s),
                std::move(t),
                std::make_shared<std::function<typename Event::signature>>([=](A... a) { return f(&*t.maybe_lock<Target>(), a...); }),
                &emit_meta_subscribed<Event>);
        }
    };
}

template <class Event, class F>
std::shared_ptr<subscription> aura::subscribe(F f) const
{
    return detail::subscribe_fwd<Event>(detail::weak_store(au_), f);
}

template <class Event, class Target, class F>
std::shared_ptr<subscription> aura::subscribe(Target * t, F f) const
{
    return detail::bind_front<Event, Target, F, typename Event::signature>::subscribe_fwd(detail::weak_store(au_), detail::weak_store(t), f);
}

template <class Event, class Target, class F>
std::shared_ptr<subscription> aura::subscribe(std::weak_ptr<Target> const & t, F f) const
{
    return detail::bind_front<Event, Target, F, typename Event::signature>::subscribe_fwd(detail::weak_store(au_), detail::weak_store(t), f);
}

template <class Event, class Target, class F>
std::shared_ptr<subscription> aura::subscribe(std::shared_ptr<Target> const & t, F f) const
{
    return detail::bind_front<Event, Target, F, typename Event::signature>::subscribe_fwd(detail::weak_store(au_), detail::weak_store(t), f);
}

template <class Event, class F>
std::shared_ptr<subscription> meta::subscribe(F f) const
{
    return detail::subscribe_fwd<Event>(detail::weak_store(detail::meta_source()), f);
}

} }

#endif
