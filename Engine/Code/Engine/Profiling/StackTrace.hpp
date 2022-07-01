#pragma once

#include "Engine/Core/BuildConfig.hpp"

#include <array>
#include <atomic>
#include <shared_mutex>
#include <source_location>
#include <string>
#include <vector>

class StackTrace final {
public:
    StackTrace(std::source_location sloc = std::source_location::current()) noexcept;
    StackTrace([[maybe_unused]] unsigned long framesToSkip,
               [[maybe_unused]] unsigned long framesToCapture,
               [[maybe_unused]] std::source_location sloc = std::source_location::current()) noexcept;
    ~StackTrace() noexcept;
    [[nodiscard]] bool operator==(const StackTrace& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const StackTrace& rhs) const noexcept;

protected:
private:
    static void Initialize() noexcept;
    static void Shutdown() noexcept;
    static void GetLines([[maybe_unused]] StackTrace* st, [[maybe_unused]] unsigned long max_lines) noexcept;
    unsigned long m_hash = 0;
    unsigned long m_frame_count = 0;
    std::source_location m_current_source_location{};
    static constexpr auto m_max_frames_per_callstack = 128ul;
    std::array<void*, m_max_frames_per_callstack> m_frames{};
    static std::shared_mutex m_cs;
    static std::atomic_uint64_t m_refs;
    static std::atomic_bool m_did_init;
};

#ifdef PROFILE_BUILD
    #undef UNIQUE_STACKTRACE
    #define UNIQUE_STACKTRACE() \
        { static StackTrace TOKEN_PASTE(st, __LINE__); }
    #define STACKTRACE() \
        { StackTrace TOKEN_PASTE(st, __LINE__); }
    #define STACKTRACE_WITH_ARGS(skip, capture) \
        { StackTrace TOKEN_PASTE(st, __LINE__)(skip, capture); }
    #define UNIQUE_STACKTRACE_WITH_ARGS(skip, capture) \
        { static StackTrace TOKEN_PASTE(st, __LINE__)(skip, capture); }
#else
    #undef UNIQUE_STACKTRACE_WITH_ARGS
    #define UNIQUE_STACKTRACE_WITH_ARGS(skip, capture)
    #undef STACKTRACE_WITH_ARGS
    #define STACKTRACE_WITH_ARGS()
    #undef STACKTRACE
    #define STACKTRACE()
    #undef UNIQUE_STACKTRACE
    #define UNIQUE_STACKTRACE()
#endif