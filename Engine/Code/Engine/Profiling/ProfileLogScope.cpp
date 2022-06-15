#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/TimeUtils.hpp"

ProfileLogScope::ProfileLogScope(const char* scopeName) noexcept
: m_scope_name(scopeName)
, m_time_at_creation(TimeUtils::Now()) {
    /* DO NOTHING */
}

ProfileLogScope::~ProfileLogScope() noexcept {
    const auto now = TimeUtils::Now();
    TimeUtils::FPMilliseconds elapsedTime = (now - m_time_at_creation);
    DebuggerPrintf(std::format("ProfileLogScope {} took {:.2f}.\n", m_scope_name, elapsedTime.count()));
}
