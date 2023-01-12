#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <format>

ProfileLogScope::ProfileLogScope(const char* scopeName, std::source_location location) noexcept
: m_scope_name(scopeName)
, m_time_at_creation(TimeUtils::Now())
, m_location(location)
{
    /* DO NOTHING */
}

ProfileLogScope::~ProfileLogScope() noexcept {
    const auto now = TimeUtils::Now();
    TimeUtils::FPMilliseconds elapsedTime = (now - m_time_at_creation);
    DebuggerPrintf(std::format("ProfileLogScope {} in file {} on line {} took {:.2f} milliseconds.\n", m_scope_name != nullptr ? m_scope_name : m_location.function_name(), m_location.file_name(), m_location.line(), elapsedTime.count()));
}
