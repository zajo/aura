// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/emit.hpp>
#include "boost/core/lightweight_test.hpp"

using aura = boost::aurae::aura;

struct test_event: aura::event<void(int)> {};

int main(int argc, char const * argv[])
{
    int s;
    BOOST_TEST_EQ(aura(&s).emit<test_event>(42), 0);
    return boost::report_errors();
}
