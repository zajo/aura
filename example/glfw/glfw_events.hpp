// Copyright (c) 2026 Emil Dotchevski

// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This header defines aura events for use with GLFW.

#ifndef UUID_DF9B5300F10111E79DCEF6AC7105BEE2
#define UUID_DF9B5300F10111E79DCEF6AC7105BEE2

#include <boost/aura/event.hpp>

struct GLFWwindow;

namespace glfw_events
{
    using aura = boost::aurae::aura;

    // User input callbacks
    struct Key: aura::event<void(GLFWwindow *, int, int, int, int)> {};
    struct Char: aura::event<void(GLFWwindow *, unsigned int)> {};
    struct CharMods: aura::event<void(GLFWwindow *, unsigned int, int)> {};
    struct CursorPos: aura::event<void(GLFWwindow *, double, double)> {};
    struct CursorEnter: aura::event<void(GLFWwindow *, int)> {};
    struct MouseButton: aura::event<void(GLFWwindow *, int, int, int)> {};
    struct Scroll: aura::event<void(GLFWwindow *, double, double)> {};
    struct Drop: aura::event<void(GLFWwindow *, int, char const * *)> {};

    // Window state callbacks
    struct WindowClose: aura::event<void(GLFWwindow *)> {};
    struct WindowSize: aura::event<void(GLFWwindow *, int, int)> {};
    struct FramebufferSize: aura::event<void(GLFWwindow *, int, int)> {};
    struct WindowPos: aura::event<void(GLFWwindow *, int, int)> {};
    struct WindowIconify: aura::event<void(GLFWwindow *, int)> {};
    struct WindowFocus: aura::event<void(GLFWwindow *, int)> {};
    struct WindowRefresh: aura::event<void(GLFWwindow *)> {};

    // This is emitted from the GLFWwindow object to report exceptions from subscribed event handlers
    struct exception_caught: aura::event<void(GLFWwindow *)> {};
}

#endif
