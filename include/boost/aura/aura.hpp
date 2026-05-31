#ifndef BOOST_AURA_AURA_HPP_INCLUDED
#define BOOST_AURA_AURA_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/event.hpp>
#include <boost/aura/attribute.hpp>
#include <boost/aura/detail/weak_store.hpp>
#include <memory>

namespace boost { namespace aurae {

class subscription;
class muting;
class attachment;
struct meta;

class aura
{
    aura(aura const &) = delete;
    aura & operator=(aura const &) = delete;

    detail::weak_store au_;

public:

    template <class Signature, thread_affinity Affinity = thread_affinity::emitting_thread> using event = aurae::event<Signature, Affinity>;
    template <class T, thread_affinity Affinity = thread_affinity::emitting_thread> using attribute = aurae::attribute<T, Affinity>;
    using thread_affinity = aurae::thread_affinity;
    using subscription = aurae::subscription;
    using muting = aurae::muting;
    using attachment = aurae::attachment;
    using meta = aurae::meta;

    template <class T>
    aura(T * x)
    {
        if (x)
            au_ = detail::weak_store(x);
    }

    template <class T>
    aura(std::shared_ptr<T> const & sp)
    {
        if (sp)
            au_ = detail::weak_store(sp);
    }

    template <class T>
    aura(std::weak_ptr<T> const & wp)
    {
        if (wp.lock())
            au_ = detail::weak_store(wp);
    }

    template <class Event, class... A>
    int emit(A && ...) const;

    template <class Event, class F>
    std::shared_ptr<subscription> subscribe(F) const;

    template <class Event, class Target, class F>
    std::shared_ptr<subscription> subscribe(Target *, F) const;

    template <class Event, class Target, class F>
    std::shared_ptr<subscription> subscribe(std::weak_ptr<Target> const &, F) const;

    template <class Event, class Target, class F>
    std::shared_ptr<subscription> subscribe(std::shared_ptr<Target> const &, F) const;

    template <class Event>
    std::shared_ptr<muting> mute() const;

    template <class Attr>
    std::shared_ptr<attachment> attach(typename Attr::value_type const & = typename Attr::value_type()) const;

    template <class Attr>
    int set(typename Attr::value_type const &) const;

    template <class Attr>
    typename Attr::value_type get(typename Attr::value_type const & = typename Attr::value_type()) const;
};

} }

#endif
