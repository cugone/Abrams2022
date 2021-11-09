#pragma once
#ifndef ABRAMS2022_BUILDCONFIG_HPP
    #define ABRAMS2022_BUILDCONFIG_HPP

    #define TRACK_MEMORY_BASIC (0)
    #define TRACK_MEMORY_VERBOSE (1)

    #define UNUSED(x) (void)(x)

    #ifdef _DEBUG
        #define TRACK_MEMORY TRACK_MEMORY_VERBOSE
        #define DEBUG_BUILD
        #define AUDIO_DEBUG
        #define RENDER_DEBUG
        #define PHYSICS_DEBUG
        #define UI_DEBUG
        #define PROFILE_BUILD
        #undef DISABLE_ASSERTS
    #elif defined(FINAL_BUILD)
        #ifndef DISABLE_ASSERTS
            #define DISABLE_ASSERTS
        #endif
        #undef TRACK_MEMORY
        #undef DEBUG_BUILD
        #undef AUDIO_DEBUG
        #undef RENDER_DEBUG
        #undef PHYSICS_DEBUG
        #undef UI_DEBUG
        #undef PROFILE_BUILD
    #else
        #define TRACK_MEMORY TRACK_MEMORY_BASIC
        #define DEBUG_BUILD
        #define AUDIO_DEBUG
        #define RENDER_DEBUG
        #define PHYSICS_DEBUG
        #define UI_DEBUG
        #define PROFILE_BUILD
        #undef DISABLE_ASSERTS
    #endif

    #define MAX_LOGS 3u

    #define TOKEN_PASTE_SIMPLE(x, y) x##y
    #define TOKEN_PASTE(x, y) TOKEN_PASTE_SIMPLE(x, y)
    #define TOKEN_STRINGIZE_SIMPLE(x) #x
    #define TOKEN_STRINGIZE(x) TOKEN_STRINGIZE_SIMPLE(x)

    #if !defined(__cplusplus)
        #error "This program only compiles in C++"
    #endif

    #if defined(__clang__)
        #ifndef PLATFORM_CLANG
            #define PLATFORM_CLANG
        #endif
    #elif defined(__GNUC__) || defined(__GNUG__)
        #ifndef PLATFORM_GNUC
            #define PLATFORM_GNUC
        #endif
    #endif

    #if defined(__APPLE__) || defined(__MACH__)
        #error "Apple or Mac-specific compilers are not supported."
    #elif !(defined(_WIN64) || defined(_WIN32))
        #ifndef PLATFORM_LINUX
            #define PLATFORM_LINUX
        #endif
    #elif defined(_WIN64) || defined(_WIN32)
        #ifndef PLATFORM_WINDOWS
            #define PLATFORM_WINDOWS
        #endif
    #else
        #error "Unknown or unsupported platform."
    #endif

#endif
