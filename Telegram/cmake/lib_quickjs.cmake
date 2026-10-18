# This file is part of Nexo Desk, based on AyuGram Desktop and
# Telegram Desktop. Licensed under GPLv3, see LEGAL.
#
# QuickJS-ng (MIT) for Nexo Desk plugins, pinned as a submodule in
# ThirdParty/quickjs. Only the core engine is built: quickjs-libc
# (files, OS, workers) is left out on purpose, so plugins get no file
# or OS access from the engine itself.

add_library(lib_quickjs STATIC)
add_library(desktop-app::lib_quickjs ALIAS lib_quickjs)

set(quickjs_loc ${third_party_loc}/quickjs)

target_sources(lib_quickjs
PRIVATE
    ${quickjs_loc}/dtoa.c
    ${quickjs_loc}/libregexp.c
    ${quickjs_loc}/libunicode.c
    ${quickjs_loc}/quickjs.c
)

target_include_directories(lib_quickjs SYSTEM
PUBLIC
    ${quickjs_loc}
)

target_compile_definitions(lib_quickjs
PRIVATE
    _GNU_SOURCE
    $<$<BOOL:${WIN32}>:WIN32_LEAN_AND_MEAN>
    $<$<BOOL:${WIN32}>:_WIN32_WINNT=0x0601>
)

set_target_properties(lib_quickjs PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if (MSVC)
    set_target_properties(lib_quickjs PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    target_compile_options(lib_quickjs
    PRIVATE
        /experimental:c11atomics
        /W0
    )
endif()
