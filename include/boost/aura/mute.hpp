#ifndef BOOST_AURA_MUTE_HPP_INCLUDED
#define BOOST_AURA_MUTE_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/emit.hpp>
#include <utility>

namespace boost { namespace aurae {

class muting:
    public detail::subscription_base
{
    muting(muting const &);
    muting & operator=(muting const &);

public:

    explicit muting(std::shared_ptr<detail::subscription_list> const & sl):
        detail::subscription_base(sl)
    {
    }

    ~muting();

    template <class T>
    std::shared_ptr<T> source() const
    {
        return source_().maybe_lock<T>();
    }
};

std::weak_ptr<muting const> persist(std::shared_ptr<muting const> const &);
std::weak_ptr<muting> persist(std::shared_ptr<muting> const &);

std::shared_ptr<muting const> release(std::weak_ptr<muting const> const &);
std::shared_ptr<muting> release(std::weak_ptr<muting> const &);

////////////////////////////////////////

namespace detail
{
    template <class Event>
    int emit_meta_muted(subscription_base & sb, unsigned context)
    {
        return emit_dispatch<meta::muted<Event>>(meta_source().get(), std::ref(static_cast<muting &>(sb)), context);
    }

    std::shared_ptr<muting> mute_(std::shared_ptr<thread_local_event_data> const &, weak_store && s, emit_meta_fn emit_meta);

    template <class Event>
    std::shared_ptr<muting> mute_fwd(weak_store && s)
    {
        return mute_(get_thread_local_event_data<Event>(true), std::move(s), &emit_meta_muted<Event>);
    }
}

template <class Event>
std::shared_ptr<muting> aura::mute() const
{
    return detail::mute_fwd<Event>(detail::weak_store(au_));
}

} }

#endif
