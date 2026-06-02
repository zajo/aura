// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/attach.hpp>
#include "boost/core/lightweight_test.hpp"
#include <memory>
#include <string>

using aura = boost::aurae::aura;

struct attr1: aura::attribute<std::string> {};

int main(int, char const *[])
{
    {
        int obj;
        BOOST_TEST_EQ(aura(&obj).get<attr1>("default"), "default");
        auto a = aura(&obj).attach<attr1>("Hello World");
        BOOST_TEST(a);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Hello World");
        BOOST_TEST_EQ(aura(&obj).set<attr1>("Reset World"), 1);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Reset World");
        a.reset();
        BOOST_TEST(aura(&obj).get<attr1>().empty());
    }
    {
        int obj;
        std::shared_ptr<int> sp(&obj, [](int*){});
        BOOST_TEST_EQ(aura(&obj).get<attr1>("default"), "default");
        auto a = aura(sp).attach<attr1>("Hello Shared");
        BOOST_TEST(a);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Hello Shared");
        BOOST_TEST_EQ(aura(sp).set<attr1>("Reset Shared"), 1);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Reset Shared");
        sp.reset();
        BOOST_TEST_EQ(aura(&obj).get<attr1>("gone"), "gone");
    }
    {
        int obj;
        std::shared_ptr<int> sp(&obj, [](int*){});
        std::weak_ptr<int> wp = sp;
        BOOST_TEST_EQ(aura(&obj).get<attr1>("default"), "default");
        auto a = aura(wp).attach<attr1>("Hello Weak");
        BOOST_TEST(a);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Hello Weak");
        BOOST_TEST_EQ(aura(wp).set<attr1>("Reset Weak"), 1);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "Reset Weak");
        sp.reset();
        BOOST_TEST_EQ(aura(&obj).get<attr1>("gone"), "gone");
    }
    {
        std::weak_ptr<int> wp;
        {
            auto sp = std::make_shared<int>();
            wp = sp;
        }
        BOOST_TEST(wp.expired());
        auto a = aura(wp).attach<attr1>("ignored");
        BOOST_TEST(!a);
    }
    {
        int obj;
        BOOST_TEST_EQ(aura(&obj).set<attr1>("no-slot"), 0);
        BOOST_TEST_EQ(aura(&obj).get<attr1>("default"), "default");
    }
    {
        int obj;
        auto a1 = aura(&obj).attach<attr1>("first");
        BOOST_TEST(a1);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "first");
        auto a2 = aura(&obj).attach<attr1>("ignored");
        BOOST_TEST(a2);
        BOOST_TEST(a1 == a2);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "first");
        a1.reset();
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "first");
        BOOST_TEST_EQ(aura(&obj).set<attr1>("updated"), 1);
        BOOST_TEST_EQ(aura(&obj).get<attr1>(), "updated");
        a2.reset();
        BOOST_TEST(aura(&obj).get<attr1>().empty());
    }
    {
        int obj1;
        int obj2;
        auto a1 = aura(&obj1).attach<attr1>("one");
        auto a2 = aura(&obj2).attach<attr1>("two");
        BOOST_TEST(a1);
        BOOST_TEST(a2);
        BOOST_TEST(a1 != a2);
        BOOST_TEST_EQ(aura(&obj1).get<attr1>(), "one");
        BOOST_TEST_EQ(aura(&obj2).get<attr1>(), "two");
        BOOST_TEST_EQ(aura(&obj1).set<attr1>("one2"), 1);
        BOOST_TEST_EQ(aura(&obj1).get<attr1>(), "one2");
        BOOST_TEST_EQ(aura(&obj2).get<attr1>(), "two");
    }
    return boost::report_errors();
}
