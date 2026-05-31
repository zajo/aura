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
    template <class T>
    struct arg_storage
    {
        typedef typename std::decay<T>::type type;
    };

    template <class T>
    struct arg_storage<T &>
    {
        typedef std::reference_wrapper<T> type;
    };

    template <class T>
    struct arg_storage<T const &>
    {
        typedef T type;
    };

    class args_binder_base
    {
    public:

        virtual std::shared_ptr<args_binder_base> clone() const = 0;
        virtual void call(void const *) const = 0;
    };

    template <class Signature>
    class args_binder;

    template <class... SigA>
    class args_binder<void(SigA...)>:
        public args_binder_base
    {
        std::tuple<typename arg_storage<SigA>::type...> a_;

        std::shared_ptr<args_binder_base> clone() const final override
        {
            return std::make_shared<args_binder>(*this);
        }

        void call(void const * f) const final override
        {
            detail_mp11::tuple_apply(*static_cast<std::function<void(SigA...)> const *>(f), a_);
        }

    public:

        template <class... CallA>
        args_binder(CallA && ... a):
            a_(std::forward<CallA>(a)...)
        {
        }
    };

    template <class Event>
    int emit_fwd(void const * s, args_binder_base const & args)
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
        return emit_fwd<Event>(s, args_binder<typename Event::signature>(std::forward<A>(a)...));
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
