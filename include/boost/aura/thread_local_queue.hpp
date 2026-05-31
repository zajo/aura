#ifndef BOOST_AURA_THREAD_LOCAL_QUEUE_HPP_INCLUDED
#define BOOST_AURA_THREAD_LOCAL_QUEUE_HPP_INCLUDED

// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/aura/config.hpp>
#include <memory>
#include <functional>

namespace boost { namespace aurae {

class thread_local_queue
{
protected:

    thread_local_queue();
    ~thread_local_queue();

public:

    virtual int poll() = 0;
    virtual int wait() = 0;
    virtual void post(std::function<void()> const &) = 0;
};

std::shared_ptr<thread_local_queue> create_thread_local_queue();

} }

#endif
