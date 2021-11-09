#pragma once

#include "Engine/Core/BuildConfig.hpp"

#include <chrono>

class ProfileLogScope {
public:
    explicit ProfileLogScope(const char* scopeName) noexcept;
    ~ProfileLogScope() noexcept;

    ProfileLogScope() = delete;
    ProfileLogScope(const ProfileLogScope&) = delete;
    ProfileLogScope(ProfileLogScope&&) = delete;
    ProfileLogScope& operator=(const ProfileLogScope&) = delete;
    ProfileLogScope& operator=(ProfileLogScope&&) = delete;

protected:
private:
    using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

    const char* _scope_name = nullptr;
    time_point_t _time_at_creation{};
};

#if defined PROFILE_LOG_SCOPE || defined PROFILE_LOG_SCOPE_FUNCTION
    #undef PROFILE_LOG_SCOPE
    #undef PROFILE_LOG_SCOPE_FUNCTION
#endif
#ifdef PROFILE_BUILD
    #define PROFILE_LOG_SCOPE(tag_str) ProfileLogScope TOKEN_PASTE(plscope_, __LINE__)(tag_str)
    #define PROFILE_LOG_SCOPE_FUNCTION() PROFILE_LOG_SCOPE(__FUNCSIG__)
#else
    #define PROFILE_LOG_SCOPE(tag_str)
    #define PROFILE_LOG_SCOPE_FUNCTION()
#endif
