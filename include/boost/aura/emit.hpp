#ifndef BOOST_AURA_EMIT_HPP_INCLUDED
#define BOOST_AURA_EMIT_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <boost/aura/aura.hpp>
#include <boost/aura/detail/common.hpp>
#include <boost/aura/detail/mp11.hpp>
#include <functional>
#include <type_traits>
#include <utility>

namespace boost { namespace aurae {

namespace detail
{
    class args_binder
    {
    public:

        virtual void call(void const *) const = 0;
    };

    class emit_args_binder:
        public args_binder
    {
    public:

        virtual std::shared_ptr<args_binder> clone() const = 0;
    };

    ////////////////////////////////////////

    template <class T>
    struct queued_arg_storage
    {
        using type = typename std::decay<T>::type;
    };

    template <class T>
    struct queued_arg_storage<T &>
    {
        using type = std::reference_wrapper<T>;
    };

    template <class T>
    struct queued_arg_storage<T const &>
    {
        using type = T;
    };

    template <class Signature>
    class queued_args_binder;

    template <class... SigA>
    class queued_args_binder<void(SigA...)> final:
        public args_binder
    {
        std::tuple<typename queued_arg_storage<SigA>::type...> a_;

        void call(void const * f) const override
        {
            detail_mp11::tuple_apply(*static_cast<std::function<void(SigA...)> const *>(f), a_);
        }

    public:

        template <class... A>
        queued_args_binder(A && ... a):
            a_(std::forward<A>(a)...)
        {
        }
    };

    ////////////////////////////////////////

    template <class A>
    struct local_arg_ref
    {
        using type = A const &;
    };

    template <class A>
    struct local_arg_ref<A &>
    {
        using type = A &;
    };

    template <class Signature, class... A>
    class local_args_binder final:
        public emit_args_binder
    {
        std::tuple<typename local_arg_ref<A>::type...> a_;

        template <std::size_t... I>
        std::shared_ptr<args_binder> clone_(detail_mp11::index_sequence<I...>) const
        {
            return std::make_shared<queued_args_binder<Signature>>(std::get<I>(a_)...);
        }

        std::shared_ptr<args_binder> clone() const override
        {
            return clone_(detail_mp11::make_index_sequence<sizeof...(A)>());
        }

        void call(void const * f) const override
        {
            detail_mp11::tuple_apply(*static_cast<std::function<Signature> const *>(f), a_);
        }

    public:

        local_args_binder(typename local_arg_ref<A>::type... a):
            a_(a...)
        {
        }
    };

    ////////////////////////////////////////

    template <class Event>
    int emit_fwd(void const * s, emit_args_binder const & args)
    {
        if (std::shared_ptr<thread_local_event_data> const & tled = get_thread_local_event_data<Event>(false))
            if (s)
                return tled->emit_(*tled, s, &args);
        return 0;
    }

    template <class Event>
    int emit_fwd_no_args(void const * s)
    {
        if (std::shared_ptr<thread_local_event_data> const & tled = get_thread_local_event_data<Event>(false))
            if (s)
                return tled->emit_(*tled, s, 0);
        return 0;
    }

    template <class Event, class... A>
    int emit_dispatch(void const * s, A && ... a)
    {
        return emit_fwd<Event>(s, local_args_binder<typename Event::signature, A...>(std::forward<A>(a)...));
    }
}

template <class Event, class... A>
int aura::emit(A && ... a) const
{
    if (std::shared_ptr<void const> sp = au_.maybe_lock<void const>())
        return detail::emit_dispatch<Event>(sp.get(), std::forward<A>(a)...);
    return 0;
}

} }

#endif
