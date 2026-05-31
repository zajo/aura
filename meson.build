# Copyright (c) 2026 Emil Dotchevski

# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

project('aura', 'cpp', default_options : ['cpp_std=c++17', 'b_pch=false'], license : 'boost')

option_enable_unit_tests = get_option('aura_enable_unit_tests')

compiler = meson.get_compiler('cpp')
compiler_id = compiler.get_id()
if not meson.is_subproject()
    if compiler_id=='clang'
        if get_option('buildtype')!='debug'
            add_global_arguments(
                '-Wno-unused-variable',
                language:'cpp' )
        endif
        add_global_arguments(
            '-Wno-delete-non-abstract-non-virtual-dtor',
            '-Wno-non-virtual-dtor',
            '-Wno-dangling-else',
            language:'cpp' )
    elif compiler_id=='gcc'
        if get_option('buildtype')!='debug'
            add_global_arguments(
                '-Wno-unused-variable',
                '-Wno-unused-but-set-variable',
                language:'cpp' )
        endif
        add_global_arguments(
            '-Wno-non-virtual-dtor',
            '-Wno-dangling-else',
            '-Wno-sign-compare',
            language:'cpp' )
    elif compiler_id=='msvc'
        add_global_arguments(
            language:'cpp' )
    endif
endif

dep_threads = dependency('threads')

includes = include_directories('include')

emit = declare_dependency( include_directories: includes )

thread_local_queue = declare_dependency(
    include_directories: includes,
    sources: 'src/thread_local_queue.cpp')

subscribe = declare_dependency( include_directories: includes, sources:
    'src/subscribe.cpp',
    dependencies: [ thread_local_queue ] )

all = declare_dependency( include_directories: includes,
    link_with: static_library(
        'aura',
        dependencies: [
            subscribe,
            thread_local_queue ] ) )


includes = [ include_directories('include') ]

#################################

if option_enable_unit_tests

    tests = [
        [ '_hpp_compile_attach_test', [ emit ] ],
        [ '_hpp_compile_mute_test', [ emit ] ],
        [ '_hpp_compile_subscribe_test', [ emit ] ],
        [ '_hpp_compile_emit_test', [ emit ] ],
        [ '_hpp_compile_thread_local_queue_test', [ emit ] ],
        [ 'args_subscribe_test', [ subscribe ] ],
        [ 'mute_emit_test', [ subscribe ] ],
        [ 'mute_test', [ subscribe ] ],
        [ 'subscribe_emit_test', [ subscribe ] ],
        [ 'subscription_lifetime_test', [ subscribe ] ],
        [ 'subscription_user_data_test', [ subscribe ] ],
        [ 'ct_basic_test', [ subscribe, dep_threads ] ],
        [ 'ct_lambda_test', [ subscribe, dep_threads ] ],
        [ 'ct_tlq_lifetime_test', [ subscribe, dep_threads ] ],
        [ 'ct_wait_missed_wakeup_test', [ subscribe, dep_threads ] ],
        [ 'ct_wait_test', [ subscribe, dep_threads ] ],
        [ 'emit_stub_test', [ emit ] ],
        [ 'emit_test', [ subscribe ] ],
        [ 'attr_test', [ subscribe ] ],
        [ 'ct_attr_test', [ subscribe, dep_threads ] ],
        [ 'release_test', [ subscribe ] ],
        [ 'shutdown1_test', [ subscribe ] ],
        [ 'shutdown2_test', [ subscribe ] ],
        [ 'shutdown3_test', [ subscribe ] ],
        [ 'shutdown4_test', [ subscribe ] ],
        [ 'shutdown5_test', [ subscribe ] ],
        [ 'shutdown6_test', [ subscribe ] ],
        [ 'shutdown7_test', [ subscribe ] ],
        [ 'shutdown8_test', [ subscribe ] ],
        [ 'shutdown9_test', [ subscribe ] ],
        [ 'weak_store_test', [ subscribe ] ]
    ]
    foreach t : tests
        test(t[0], executable(t[0], 'test/'+t[0]+'.cpp', dependencies: t[1]) )
    endforeach

endif

if get_option('aura_glfw_example')
    executable('glfw_example', 'example/glfw/simple.cpp', dependencies: [ subscribe, dependency('glfw3'), dependency('gl') ] )
endif
