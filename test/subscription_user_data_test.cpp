// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include "boost/core/lightweight_test.hpp"

using aura = boost::aurae::aura;
namespace
{
    struct my_source_type {};
    struct my_event: aura::event<void()> {};
    void noop()
    {
    }
}

int main(int argc, char const * argv[])
{
    {
        my_source_type s;
        std::shared_ptr<aura::subscription> c = aura(&s).subscribe<my_event>(&noop);
        BOOST_TEST(!c->get_user_data<int>());
        c->set_user_data(42);
        BOOST_TEST_EQ(*c->get_user_data<int>(), 42);
    }
    {
        auto s = std::make_shared<my_source_type>();
        std::shared_ptr<aura::subscription> c = aura(s).subscribe<my_event>(&noop);
        BOOST_TEST(!c->get_user_data<int>());
        c->set_user_data(42);
        BOOST_TEST_EQ(*c->get_user_data<int>(), 42);
    }
    return boost::report_errors();
}
