#ifndef BOOST_AURA_ATTACH_HPP_INCLUDED
#define BOOST_AURA_ATTACH_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/emit.hpp>
#include <functional>
#include <utility>

namespace boost { namespace aurae {

class attachment:
    public detail::subscription_base
{
    attachment(attachment const &);
    attachment & operator=(attachment const &);

public:

    explicit attachment(std::shared_ptr<detail::subscription_list> const & sl):
        detail::subscription_base(sl)
    {
    }

    ~attachment();

    template <class T>
    std::shared_ptr<T> source() const
    {
        return source_().maybe_lock<T>();
    }

    virtual void set(void const * value) = 0;
};

std::weak_ptr<attachment const> persist(std::shared_ptr<attachment const> const &);
std::weak_ptr<attachment> persist(std::shared_ptr<attachment> const &);

std::shared_ptr<attachment const> release(std::weak_ptr<attachment const> const &);
std::shared_ptr<attachment> release(std::weak_ptr<attachment> const &);

////////////////////////////////////////

namespace detail
{
    template <class Attr>
    struct attr_set:
        event<void(typename Attr::value_type), Attr::affinity>
    {
    };

    std::shared_ptr<attachment> find_attribute_(thread_local_event_data const &, void const * source);

    std::shared_ptr<subscription_list> get_attribute_list_(std::shared_ptr<thread_local_event_data> const & tled, emit_meta_fn emit_meta);

    template <class Attr>
    int emit_meta_attached(subscription_base & sb, unsigned context)
    {
        return emit_dispatch<meta::attached<Attr>>(meta_source().get(), std::ref(static_cast<attachment &>(sb)), context);
    }

    template <class Attr>
    struct register_with_non_meta<meta::attached<Attr>>
    {
        static void keep_afloat(std::shared_ptr<thread_local_event_data const> const & meta)
        {
            auto main_tled = get_thread_local_event_data<attr_set<Attr>>(true);
            BOOST_AURA_ASSERT(!main_tled->keep_meta_attached_tled_afloat_);
            main_tled->keep_meta_attached_tled_afloat_ = meta;
        }
    };

    template <class Attr>
    class attribute_impl final:
        public attachment
    {
        typename Attr::value_type value_;

    public:

        attribute_impl(std::shared_ptr<subscription_list> const & sl, typename Attr::value_type const & initial):
            attachment(sl),
            value_(initial)
        {
        }

        void set(void const * v) override
        {
            value_ = *static_cast<typename Attr::value_type const *>(v);
        }

        typename Attr::value_type const & value() const
        {
            return value_;
        }
    };
}

////////////////////////////////////////

template <class Attr>
std::shared_ptr<attachment> aura::attach(typename Attr::value_type const & a) const
{
    if (std::shared_ptr<void const> sp = au_.maybe_lock<void const>())
    {
        std::shared_ptr<detail::thread_local_event_data> const & tled = detail::get_thread_local_event_data<detail::attr_set<Attr>>(true);
        if (std::shared_ptr<attachment> existing = detail::find_attribute_(*tled, sp.get()))
            return existing;
        std::shared_ptr<detail::subscription_list> sl = detail::get_attribute_list_(tled, &detail::emit_meta_attached<Attr>);
        std::shared_ptr<attachment> ap = std::make_shared<detail::attribute_impl<Attr>>(sl, a);
        attachment & ar = *ap;
        detail::register_(
            ap,
            detail::weak_store(au_),
            detail::weak_store(std::weak_ptr<attachment>(ap)),
            std::make_shared<std::function<void(typename Attr::value_type)>>([&ar](typename Attr::value_type v) { ar.set(&v); }));
        return ap;
    }
    return std::shared_ptr<attachment>();
}

template <class Attr>
int aura::set(typename Attr::value_type const & v) const
{
    return emit<detail::attr_set<Attr>>(v);
}

template <class Attr>
typename Attr::value_type aura::get(typename Attr::value_type const & def) const
{
    if (std::shared_ptr<void const> sp = au_.maybe_lock<void const>())
        if (std::shared_ptr<detail::thread_local_event_data> const & tled = detail::get_thread_local_event_data<detail::attr_set<Attr>>(false))
            if (std::shared_ptr<attachment> a = detail::find_attribute_(*tled, sp.get()))
                return static_cast<detail::attribute_impl<Attr> &>(*a).value();
    return def;
}

} }

#endif
