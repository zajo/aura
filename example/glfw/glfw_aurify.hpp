// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This header defines aura events for use with GLFW.

#include "glfw_events.hpp"
#include <boost/aura/subscribe.hpp>
#include "GLFW/glfw3.h"

namespace glfw_aurify_detail
{
    using aura = boost::aurae::aura;

    template <class Event, class Signature = typename Event::signature>
    class aurifier;

    template <class Event, class... A>
    class aurifier<Event, void(GLFWwindow *, A...)>
    {
        typedef void (*GLFWfun)(GLFWwindow *, A...);
        static GLFWfun prev_;

        // This is the handler that GLFW calls. It emits the corresponding Aura
        // event and calls the previous GLFW handler for the same event, if any.
        static void handler(GLFWwindow * w, A... a)
        {
            try
            {
                (void) aura(w).template emit<Event>(w, a...);
            }
            catch (...)
            {
                // We can't let exceptions propagate up into C code, so the window
                // emits the exception_caught event, which (if exceptions are
                // expected) should be subscribed to capture and handle the current
                // exception.
                bool handled = aura(w).template emit<glfw_events::exception_caught>(w) > 0;
                assert(handled);
            }
            if (prev_)
                prev_(w, a...);
        }

        public:

        explicit aurifier(GLFWfun (*setter)(GLFWwindow *, GLFWfun))
        {
            // Subscribe to the Aura meta::subscribed<Event> event. This event is
            // emitted when the Event is being subscribed (the user calls
            // aura(x).subscribe<Event>) or when the subscription expires. The
            // source pointer passed to subscribe (a GLFWwindow in this case) is
            // stored in the aura::subscription object passed to the lambda below,
            // and can be accessed by the subscription::source member function
            // template.
            boost::aurae::persist(aura::meta().template subscribe<aura::meta::subscribed<Event>>(
                [setter](aura::subscription & c, unsigned context)
                {
                    if (context & aura::meta::context_flags::subscribing)
                    {
                        // When the Event is being subscribed for the first time,
                        // use the GLFW API to install our handler.
                        if (context & aura::meta::context_flags::first_for_source)
                            prev_ = setter(c.source<GLFWwindow>().get(), &handler);
                    }
                    else
                    {
                        // When the last Event subscription expires, use the GLFW API
                        // to uninstall our handler and restore the previous handler.
                        if (context & aura::meta::context_flags::last_for_source)
                        {
                            GLFWfun p = setter(c.source<GLFWwindow>().get(), prev_);
                            assert(p == &handler);
                        }
                    }
                }));
        }
    };

    template <class Event, class... A>
    typename aurifier<Event, void(GLFWwindow *, A...)>::GLFWfun aurifier<Event, void(GLFWwindow *, A...)>::prev_;
}

// Install all the aura::meta::subscribed<....> handlers
glfw_aurify_detail::aurifier<glfw_events::WindowClose> s1(&glfwSetWindowCloseCallback);
glfw_aurify_detail::aurifier<glfw_events::WindowSize> s2(&glfwSetWindowSizeCallback);
glfw_aurify_detail::aurifier<glfw_events::FramebufferSize> s3(&glfwSetFramebufferSizeCallback);
glfw_aurify_detail::aurifier<glfw_events::WindowPos> s4(&glfwSetWindowPosCallback);
glfw_aurify_detail::aurifier<glfw_events::WindowIconify> s5(&glfwSetWindowIconifyCallback);
glfw_aurify_detail::aurifier<glfw_events::WindowFocus> s6(&glfwSetWindowFocusCallback);
glfw_aurify_detail::aurifier<glfw_events::WindowRefresh> s7(&glfwSetWindowRefreshCallback);
glfw_aurify_detail::aurifier<glfw_events::Key> s8(&glfwSetKeyCallback);
glfw_aurify_detail::aurifier<glfw_events::Char> s9(&glfwSetCharCallback);
glfw_aurify_detail::aurifier<glfw_events::CharMods> s10(&glfwSetCharModsCallback);
glfw_aurify_detail::aurifier<glfw_events::CursorPos> s11(&glfwSetCursorPosCallback);
glfw_aurify_detail::aurifier<glfw_events::CursorEnter> s12(&glfwSetCursorEnterCallback);
glfw_aurify_detail::aurifier<glfw_events::MouseButton> s13(&glfwSetMouseButtonCallback);
glfw_aurify_detail::aurifier<glfw_events::Scroll> s14(&glfwSetScrollCallback);
glfw_aurify_detail::aurifier<glfw_events::Drop> s15(&glfwSetDropCallback);
