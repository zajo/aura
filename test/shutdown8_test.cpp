// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/subscribe.hpp>
#include <stdlib.h>

using aura = boost::aurae::aura;

struct my_source_type {};
struct my_event: aura::event<void()> {};

int main(int argc, char const * argv[])
{
    std::shared_ptr<my_source_type> s = std::make_shared<my_source_type>();
    boost::aurae::persist(aura::meta().subscribe<aura::meta::subscribed<my_event>>([](aura::subscription &, unsigned) { }));
    auto subscribed = aura(s).subscribe<my_event>([](){ });
    exit(0);
}
