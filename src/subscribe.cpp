// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include <boost/aura/attach.hpp>
#include <boost/aura/mute.hpp>
#include <vector>
#include <deque>
#include <algorithm>
#include <functional>

namespace
{
#ifdef NDEBUG
    bool const debug = false;
#else
    bool const debug = true;
#endif
}

namespace boost { namespace aurae {

namespace detail
{
    cross_thread_interface * get_cross_thread_api();

    namespace
    {
        class sub_rec
        {
            void const * sp_;
            weak_store s_;
            weak_store t_;
            std::shared_ptr<void const> fn_;
            std::shared_ptr<subscription_base> sb_;
            int next_;

        public:

            sub_rec(weak_store && s, weak_store && t, std::shared_ptr<void const> const & fn):
                sp_(s.maybe_lock<void const>().get()),
                s_(std::move(s)),
                t_(std::move(t)),
                fn_(fn),
                next_(-1)
            {
                BOOST_AURA_ASSERT(sp_ != 0);
            }

            void persist(std::shared_ptr<subscription_base> const & sb)
            {
                sb_ = sb;
            }

            std::shared_ptr<subscription_base> release()
            {
                std::shared_ptr<subscription_base> sb;
                sb.swap(sb_);
                return sb;
            }

            weak_store const & source() const
            {
                return s_;
            }

            bool same_source(void const * s) const
            {
                return s == sp_;
            }

            weak_store const & target() const
            {
                return t_;
            }

            bool is_free() const
            {
                bool fr = (sp_ == 0);
                BOOST_AURA_ASSERT(s_.empty() == fr);
                BOOST_AURA_ASSERT(t_.expired() || !fr);
                return fr;
            }

            bool expired() const
            {
                BOOST_AURA_ASSERT(!is_free());
                return s_.expired() || t_.expired();
            }

            void clear(int first_free)
            {
                BOOST_AURA_ASSERT(first_free == -1 || first_free >= 0);
                BOOST_AURA_ASSERT(!is_free());
                sp_ = 0;
                s_.clear();
                t_.clear();
                fn_.reset();
                sb_.reset();
                next_ = first_free;
                BOOST_AURA_ASSERT(is_free());
            }

            int const & next() const
            {
                BOOST_AURA_ASSERT(next_ == -1 || next_ >= 0);
                return next_;
            }

            int & next()
            {
                BOOST_AURA_ASSERT(next_ == -1 || next_ >= 0);
                return next_;
            }

            int emit(void const * s, args_binder const * args) const
            {
                BOOST_AURA_ASSERT(!is_free());
                BOOST_AURA_ASSERT(s != 0);
                if (sp_ == s && fn_)
                    if (auto s_lk = s_.maybe_lock<void const>())
                        if (auto t_lk = t_.maybe_lock<void const>())
                        {
                            if (args)
                                args->call(fn_.get());
                            else
                                (*static_cast<std::function<void()> const *>(fn_.get()))();
                            return 1;
                        }
                return 0;
            };
        };
    }

    ////////////////////////////////////////

    class thread_local_event_data::subscription_list
    {
    public:

        typedef std::weak_ptr<subscription_list> thread_local_event_data::* slot_t;

    private:

        subscription_list(subscription_list const &);
        subscription_list & operator=(subscription_list const &);

        std::weak_ptr<thread_local_event_data> const tled_;
        slot_t const slot_;
        emit_meta_fn const emit_meta_;
        std::vector<sub_rec> sub_;
        std::vector<sub_rec> * emit_sub_ptr_;
        int first_free_;
        int first_rec_;
        int * last_next_;

        void check_invariants() const
        {
            if (debug)
            {
                BOOST_AURA_ASSERT(*last_next_ == -1);
                int count1 = 0;
                for (int i = first_free_; i != -1; i = sub_[i].next())
                {
                    BOOST_AURA_ASSERT(sub_[i].is_free());
                    ++count1;
                }
                int count2 = 0;
                bool found_last_next = false;
                for (int const * i = &first_rec_; ; )
                {
                    if (last_next_ == i)
                        found_last_next = true;
                    if (*i == -1)
                        break;
                    sub_rec const & cr = sub_[*i];
                    BOOST_AURA_ASSERT(!cr.is_free());
                    ++count2;
                    i = &cr.next();
                }
                BOOST_AURA_ASSERT(found_last_next);
                int count3 = 0;
                for (std::vector<sub_rec>::const_iterator i = sub_.begin(), ie = sub_.end(); i != ie; ++i)
                    count3 += i->is_free();
                BOOST_AURA_ASSERT(count1 == count3);
                BOOST_AURA_ASSERT(count1 + count2 == sub_.size());
            }
        }

        bool idx_valid(int idx) const
        {
            return idx >= 0 && idx < sub_.size();
        }

        static int emit_impl(thread_local_event_data const & tled, void const * s, emit_args_binder const * args)
        {
            BOOST_AURA_ASSERT(s != 0);
            int n = 0;
            if (tled.cross_thread_)
                if (cross_thread_interface * cross_thread = tled.cross_thread_->load())
                    n += cross_thread->emit(tled, s, args);
            if (std::shared_ptr<subscription_list> ml = tled.ml_.lock())
                if (ml->has_source(s))
                    return n;
            if (std::shared_ptr<subscription_list> sl = tled.sl_.lock())
                n += sl->emit_from_source(s, args);
            if (std::shared_ptr<subscription_list> al = tled.al_.lock())
                n += al->emit_from_source(s, args);
            return n;
        }

        static void cleanup_impl(thread_local_event_data const & tled)
        {
            if (std::shared_ptr<subscription_list> sl = tled.sl_.lock())
                sl->cleanup();
            if (std::shared_ptr<subscription_list> ml = tled.ml_.lock())
                ml->cleanup();
            if (std::shared_ptr<subscription_list> al = tled.al_.lock())
                al->cleanup();
        }

        void destroy()
        {
            if (std::shared_ptr<thread_local_event_data> tled = tled_.lock())
            {
                BOOST_AURA_ASSERT(tled);
                bool const is_main = (slot_ == &thread_local_event_data::sl_);
                if (is_main && tled->sl_count_)
                {
                    int const n = --(*tled->sl_count_);
                    BOOST_AURA_ASSERT(n >= 0);
                }
                (tled.get()->*slot_).reset();
            }
        }

        bool empty() const
        {
            return first_rec_ == -1;
        }

    public:

        subscription_list(std::shared_ptr<thread_local_event_data> const & tled, slot_t slot, emit_meta_fn emit_meta):
            tled_(tled),
            slot_(slot),
            emit_meta_(emit_meta),
            emit_sub_ptr_(0),
            first_free_(-1),
            first_rec_(-1),
            last_next_(&first_rec_)
        {
            BOOST_AURA_ASSERT(emit_meta != 0);
            tled->emit_ = &emit_impl;
            tled->cleanup_ = &cleanup_impl;
            check_invariants();
            if (slot == &thread_local_event_data::sl_ && tled->sl_count_)
            {
                int const n = ++(*tled->sl_count_);
                BOOST_AURA_ASSERT(n > 0);
            }
        }

        int emit_meta(subscription_base & sb, unsigned context)
        {
            return emit_meta_(sb, context);
        }

        ~subscription_list()
        {
            BOOST_AURA_ASSERT(empty());
            destroy();
        }

        weak_store const & source(int index) const
        {
            return sub_[index].source();
        }

        weak_store const & target(int index) const
        {
            return sub_[index].target();
        }

        std::shared_ptr<subscription_base> release(int index)
        {
            return sub_[index].release();
        }

        void persist(int index, std::shared_ptr<subscription_base> const & sp)
        {
            sub_[index].persist(sp);
        }

        int add(sub_rec const & r)
        {
            if (emit_sub_ptr_ && emit_sub_ptr_->empty())
                *emit_sub_ptr_ = sub_;
            int idx;
            if (first_free_ != -1)
            {
                BOOST_AURA_ASSERT(idx_valid(first_free_));
                idx = first_free_;
                sub_rec & cr = sub_[idx];
                BOOST_AURA_ASSERT(cr.is_free());
                first_free_ = cr.next();
                cr = r;
                *last_next_ = idx;
            }
            else
            {
                idx = sub_.size();
                *last_next_ = idx;
                sub_.push_back(r);
            }
            last_next_ = &sub_[idx].next();
            BOOST_AURA_ASSERT(first_free_ != idx);
            BOOST_AURA_ASSERT(!sub_[idx].is_free());
            check_invariants();
            return idx;
        }

        void remove(int idx)
        {
            check_invariants();
            BOOST_AURA_ASSERT(idx_valid(idx));
            BOOST_AURA_ASSERT(first_free_ != idx);
            if (emit_sub_ptr_ && emit_sub_ptr_->empty())
                *emit_sub_ptr_ = sub_;
            sub_rec & cr = sub_[idx];
            BOOST_AURA_ASSERT(!cr.is_free());
            int * i;
            for (i = &first_rec_; *i != -1 && *i != idx; i = &sub_[*i].next())
                { }
            int const j = (*i = cr.next());
            if (j == -1)
            {
                BOOST_AURA_ASSERT(last_next_ == &cr.next());
                last_next_ = i;
            }
            cr.clear(first_free_);
            first_free_ = idx;
            check_invariants();
        }

        template <class F>
        int enumerate_recs(F f)
        {
            check_invariants();
            if (sub_.empty())
                return 0;
            struct restore_emit_sub_ptr
            {
                std::vector<sub_rec> * & emit_sub_ptr_;
                std::vector<sub_rec> * const ptr_;
                explicit restore_emit_sub_ptr(std::vector<sub_rec> * & emit_sub_ptr): emit_sub_ptr_(emit_sub_ptr), ptr_(emit_sub_ptr_) { }
                ~restore_emit_sub_ptr() { emit_sub_ptr_ = ptr_; }
            } restore(emit_sub_ptr_);

            std::vector<sub_rec> emit_sub;
            emit_sub_ptr_ = &emit_sub;
            int counter = 0;
            for (int i = first_rec_; i != -1; )
            {
                sub_rec & cr = (emit_sub.empty() ? sub_ : emit_sub)[i];
                i = cr.next();
                counter += f(cr);
            }
            return counter;
        }

        int source_subscription_count(void const * s)
        {
            return s == 0 ? 0 : enumerate_recs([s](sub_rec const & r) { return r.same_source(s); });
        }

        std::shared_ptr<attachment> find_attribute_by_source(void const * s);
        std::shared_ptr<muting> find_muting_by_source(void const * s);
        bool has_source(void const * s) const;

        int emit_from_source(void const * s, args_binder const * args)
        {
            BOOST_AURA_ASSERT(s != 0);
            return enumerate_recs([s, args](sub_rec const & r) { return r.emit(s, args); });
        }

        void purge()
        {
            check_invariants();
            std::deque<std::shared_ptr<subscription_base>> purged;
            for (int const * i = &first_rec_; *i != -1; )
            {
                sub_rec & cr = sub_[*i];
                BOOST_AURA_ASSERT(!cr.is_free());
                if (cr.expired())
                    if (std::shared_ptr<subscription_base> c = cr.release())
                        purged.push_back(c);
                i = &cr.next();
            }
        }

        void cleanup()
        {
            check_invariants();
#ifndef BOOST_AURA_NO_EXCEPTIONS
            try
#endif
            {
                std::deque<std::shared_ptr<subscription_base>> purged;
                for (int const * i = &first_rec_; *i != -1; )
                {
                    sub_rec & cr = sub_[*i];
                    BOOST_AURA_ASSERT(!cr.is_free());
                    if (std::shared_ptr<subscription_base> c = cr.release())
                        purged.push_back(c);
                    i = &cr.next();
                }
            }
#ifndef BOOST_AURA_NO_EXCEPTIONS
            catch (...)
            {
            }
#endif
        }
    };

    ////////////////////////////////////////

    int emit_from_source(thread_local_event_data::subscription_list & cl, void const * s, args_binder const * args)
    {
        return cl.emit_from_source(s, args);
    }

    namespace
    {
        void register_cross_thread_(std::shared_ptr<thread_local_event_data> const & tled)
        {
            if (tled->cross_thread_ && !tled->cross_thread_registered_)
                if (cross_thread_interface * cross_thread = get_cross_thread_api())
                {
                    tled->cross_thread_->store(cross_thread);
                    tled->cross_thread_registered_ = true;
                    cross_thread->notify_subscription_list_created(tled);
                }
        }

        std::shared_ptr<thread_local_event_data::subscription_list>
        get_subscription_list_(std::shared_ptr<thread_local_event_data> const & tled, emit_meta_fn emit_meta)
        {
            std::shared_ptr<thread_local_event_data::subscription_list> sl = tled->sl_.lock();
            if (!sl)
            {
                std::make_shared<thread_local_event_data::subscription_list>(tled, &thread_local_event_data::sl_, emit_meta).swap(sl);
                tled->sl_ = sl;
                register_cross_thread_(tled);
            }
            return sl;
        }

        std::shared_ptr<thread_local_event_data::subscription_list>
        get_muting_list_(std::shared_ptr<thread_local_event_data> const & tled, emit_meta_fn emit_meta)
        {
            std::shared_ptr<thread_local_event_data::subscription_list> ml = tled->ml_.lock();
            if (!ml)
            {
                std::make_shared<thread_local_event_data::subscription_list>(tled, &thread_local_event_data::ml_, emit_meta).swap(ml);
                tled->ml_ = ml;
            }
            return ml;
        }
    }

    std::shared_ptr<thread_local_event_data::subscription_list>
    get_attribute_list_(std::shared_ptr<thread_local_event_data> const & tled, emit_meta_fn emit_meta)
    {
        std::shared_ptr<thread_local_event_data::subscription_list> al = tled->al_.lock();
        if (!al)
        {
            std::make_shared<thread_local_event_data::subscription_list>(tled, &thread_local_event_data::al_, emit_meta).swap(al);
            tled->al_ = al;
            register_cross_thread_(tled);
        }
        return al;
    }

    void register_(std::shared_ptr<subscription_base> const & sub, weak_store && s, weak_store && t, std::shared_ptr<void const> const & fn)
    {
        BOOST_AURA_ASSERT(sub);
        BOOST_AURA_ASSERT(sub->position_ == -1);
        (void) sub->sl_->purge();
        sub->position_ = sub->sl_->add(sub_rec(std::move(s), std::move(t), fn));
        unsigned context = meta::context_flags::subscribing;
        if (sub->sl_->source_subscription_count(sub->source_().maybe_lock<void const>().get()) == 1)
            context |= meta::context_flags::first_for_source;
        sub->sl_->emit_meta(*sub, context);
    }

    std::shared_ptr<subscription> subscribe_(std::shared_ptr<thread_local_event_data> const & tled, weak_store && s, weak_store && t, std::shared_ptr<void const> const & fn, emit_meta_fn emit_meta)
    {
        std::shared_ptr<subscription> c = std::make_shared<subscription>(get_subscription_list_(tled, emit_meta));
        weak_store t2 = t.empty() ? weak_store(std::weak_ptr<subscription>(c)) : std::move(t);
        register_(c, std::move(s), std::move(t2), fn);
        return c;
    }

    std::shared_ptr<attachment> thread_local_event_data::subscription_list::find_attribute_by_source(void const * s)
    {
        if (s)
            for (int idx = first_rec_; idx != -1; idx = sub_[idx].next())
            {
                sub_rec const & cr = sub_[idx];
                if (cr.same_source(s))
                    if (cr.source().maybe_lock<void const>())
                        if (std::shared_ptr<attachment> sp = cr.target().maybe_lock<attachment>())
                            return sp;
            }
        return std::shared_ptr<attachment>();
    }

    std::shared_ptr<muting> thread_local_event_data::subscription_list::find_muting_by_source(void const * s)
    {
        if (s)
            for (int idx = first_rec_; idx != -1; idx = sub_[idx].next())
            {
                sub_rec const & cr = sub_[idx];
                if (cr.same_source(s))
                    if (cr.source().maybe_lock<void const>())
                        if (std::shared_ptr<muting> sp = cr.target().maybe_lock<muting>())
                            return sp;
            }
        return std::shared_ptr<muting>();
    }

    bool thread_local_event_data::subscription_list::has_source(void const * s) const
    {
        if (s)
            for (int idx = first_rec_; idx != -1; idx = sub_[idx].next())
            {
                sub_rec const & cr = sub_[idx];
                if (cr.same_source(s))
                    if (cr.source().maybe_lock<void const>())
                        return true;
            }
        return false;
    }

    bool is_muted_(thread_local_event_data const & tled, void const * s)
    {
        if (std::shared_ptr<thread_local_event_data::subscription_list> ml = tled.ml_.lock())
            return ml->has_source(s);
        return false;
    }

    std::shared_ptr<attachment> find_attribute_(thread_local_event_data const & tled, void const * s)
    {
        if (std::shared_ptr<thread_local_event_data::subscription_list> al = tled.al_.lock())
            return al->find_attribute_by_source(s);
        return std::shared_ptr<attachment>();
    }

    std::shared_ptr<muting> mute_(std::shared_ptr<thread_local_event_data> const & tled, weak_store && s, emit_meta_fn emit_meta)
    {
        if (std::shared_ptr<void const> sp = s.maybe_lock<void const>())
        {
            std::shared_ptr<subscription_list> ml = get_muting_list_(tled, emit_meta);
            if (std::shared_ptr<muting> existing = ml->find_muting_by_source(sp.get()))
                return existing;
            std::shared_ptr<muting> m = std::make_shared<muting>(ml);
            register_(m, std::move(s), weak_store(std::weak_ptr<muting>(m)), std::shared_ptr<void const>());
            return m;
        }
        return std::shared_ptr<muting>();
    }

    std::shared_ptr<void const> & meta_source()
    {
        static std::shared_ptr<void const> me(std::make_shared<int>(42));
        return me;
    }

    subscription_base::subscription_base(std::shared_ptr<subscription_list> const & sl):
        sl_(sl),
        position_(-1)
    {
    }

    subscription_base::~subscription_base()
    {
        BOOST_AURA_ASSERT(position_ == -1);
    }

    void subscription_base::unregister_()
    {
        if (position_ != -1)
        {
            unsigned context = 0;
            if (sl_->source_subscription_count(source_().maybe_lock<void const>().get()) == 1)
                context |= meta::context_flags::last_for_source;
#ifndef BOOST_AURA_NO_EXCEPTIONS
            try
#endif
            {
                sl_->emit_meta(*this, context);
            }
#ifndef BOOST_AURA_NO_EXCEPTIONS
            catch (...)
            {
            }
#endif
            sl_->remove(position_);
            position_ = -1;
        }
    }

    weak_store const & subscription_base::source_() const
    {
        return sl_->source(position_);
    }

    std::weak_ptr<subscription_base const> persist(std::shared_ptr<subscription_base const> const & sp)
    {
        if (!sp)
            return std::weak_ptr<subscription_base const>();
        std::shared_ptr<subscription_base> nc = std::const_pointer_cast<subscription_base>(sp);
        nc->sl_->persist(nc->position_, nc);
        return sp;
    }

    std::shared_ptr<subscription_base const> release(std::weak_ptr<subscription_base const> const & wp)
    {
        std::shared_ptr<subscription_base const> sp = wp.lock();
        if (!sp)
            return std::shared_ptr<subscription_base const>();
        std::shared_ptr<subscription_base> nc = std::const_pointer_cast<subscription_base>(sp);
        if (nc->position_ == -1)
            return std::shared_ptr<subscription_base const>();
        nc->sl_->release(nc->position_);
        return sp;
    }
}

subscription::~subscription()
{
    unregister_();
}

detail::weak_store const & subscription::target_() const
{
    return sl_->target(position_);
}

attachment::~attachment()
{
    unregister_();
}

muting::~muting()
{
    unregister_();
}

std::weak_ptr<subscription const> persist(std::shared_ptr<subscription const> const & sp)
{
    std::weak_ptr<detail::subscription_base const> wb = detail::persist(std::shared_ptr<detail::subscription_base const>(sp));
    if (std::shared_ptr<detail::subscription_base const> b = wb.lock())
        return std::shared_ptr<subscription const>(b, static_cast<subscription const *>(b.get()));
    return std::weak_ptr<subscription const>();
}

std::weak_ptr<subscription> persist(std::shared_ptr<subscription> const & sp)
{
    std::shared_ptr<subscription const> r = persist(std::shared_ptr<subscription const>(sp)).lock();
    return std::const_pointer_cast<subscription>(r);
}

std::shared_ptr<subscription const> release(std::weak_ptr<subscription const> const & wp)
{
    std::shared_ptr<detail::subscription_base const> b = detail::release(std::weak_ptr<detail::subscription_base const>(wp));
    if (b)
        return std::shared_ptr<subscription const>(b, static_cast<subscription const *>(b.get()));
    return std::shared_ptr<subscription const>();
}

std::shared_ptr<subscription> release(std::weak_ptr<subscription> const & wp)
{
    std::shared_ptr<subscription const> r = release(std::weak_ptr<subscription const>(wp));
    return std::const_pointer_cast<subscription>(r);
}

std::weak_ptr<attachment const> persist(std::shared_ptr<attachment const> const & ap)
{
    std::weak_ptr<detail::subscription_base const> wb = detail::persist(std::shared_ptr<detail::subscription_base const>(ap));
    if (std::shared_ptr<detail::subscription_base const> b = wb.lock())
        return std::shared_ptr<attachment const>(b, static_cast<attachment const *>(b.get()));
    return std::weak_ptr<attachment const>();
}

std::weak_ptr<attachment> persist(std::shared_ptr<attachment> const & ap)
{
    std::shared_ptr<attachment const> r = persist(std::shared_ptr<attachment const>(ap)).lock();
    return std::const_pointer_cast<attachment>(r);
}

std::shared_ptr<attachment const> release(std::weak_ptr<attachment const> const & wp)
{
    std::shared_ptr<detail::subscription_base const> b = detail::release(std::weak_ptr<detail::subscription_base const>(wp));
    if (b)
        return std::shared_ptr<attachment const>(b, static_cast<attachment const *>(b.get()));
    return std::shared_ptr<attachment const>();
}

std::shared_ptr<attachment> release(std::weak_ptr<attachment> const & wp)
{
    std::shared_ptr<attachment const> r = release(std::weak_ptr<attachment const>(wp));
    return std::const_pointer_cast<attachment>(r);
}

std::weak_ptr<muting const> persist(std::shared_ptr<muting const> const & mp)
{
    std::weak_ptr<detail::subscription_base const> wb = detail::persist(std::shared_ptr<detail::subscription_base const>(mp));
    if (std::shared_ptr<detail::subscription_base const> b = wb.lock())
        return std::shared_ptr<muting const>(b, static_cast<muting const *>(b.get()));
    return std::weak_ptr<muting const>();
}

std::weak_ptr<muting> persist(std::shared_ptr<muting> const & mp)
{
    std::shared_ptr<muting const> r = persist(std::shared_ptr<muting const>(mp)).lock();
    return std::const_pointer_cast<muting>(r);
}

std::shared_ptr<muting const> release(std::weak_ptr<muting const> const & wp)
{
    std::shared_ptr<detail::subscription_base const> b = detail::release(std::weak_ptr<detail::subscription_base const>(wp));
    if (b)
        return std::shared_ptr<muting const>(b, static_cast<muting const *>(b.get()));
    return std::shared_ptr<muting const>();
}

std::shared_ptr<muting> release(std::weak_ptr<muting> const & wp)
{
    std::shared_ptr<muting const> r = release(std::weak_ptr<muting const>(wp));
    return std::const_pointer_cast<muting>(r);
}

} }
