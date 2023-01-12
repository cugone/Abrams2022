#pragma once

#include "Engine/Core/BuildConfig.hpp"

#include <chrono>
#include <source_location>

class ProfileLogScope {
public:
    explicit ProfileLogScope(const char* scopeName = nullptr, std::source_location location = std::source_location::current()) noexcept;
    ~ProfileLogScope() noexcept;

    ProfileLogScope() = delete;
    ProfileLogScope(const ProfileLogScope&) = delete;
    ProfileLogScope(ProfileLogScope&&) = delete;
    ProfileLogScope& operator=(const ProfileLogScope&) = delete;
    ProfileLogScope& operator=(ProfileLogScope&&) = delete;

protected:
private:
    using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

    const char* m_scope_name = nullptr;
    time_point_t m_time_at_creation{};
    std::source_location m_location{};
};

#if defined PROFILE_LOG_SCOPE || defined PROFILE_LOG_SCOPE_FUNCTION
    #undef PROFILE_LOG_SCOPE
    #undef PROFILE_LOG_SCOPE_FUNCTION
#endif
#ifdef PROFILE_BUILD
    #define PROFILE_LOG_SCOPE(tag_str) auto TOKEN_PASTE(plscope_, __LINE__) = ProfileLogScope{tag_str}
    #define PROFILE_LOG_SCOPE_FUNCTION() auto TOKEN_PASTE(plscope_, __LINE__) = ProfileLogScope{nullptr}
#else
    #define PROFILE_LOG_SCOPE(tag_str)
    #define PROFILE_LOG_SCOPE_FUNCTION()
#endif
