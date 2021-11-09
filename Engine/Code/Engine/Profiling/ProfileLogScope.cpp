#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/TimeUtils.hpp"

ProfileLogScope::ProfileLogScope(const char* scopeName) noexcept
: _scope_name(scopeName)
, _time_at_creation(TimeUtils::Now()) {
    /* DO NOTHING */
}

ProfileLogScope::~ProfileLogScope() noexcept {
    const auto now = TimeUtils::Now();
    TimeUtils::FPMilliseconds elapsedTime = (now - _time_at_creation);
    DebuggerPrintf("ProfileLogScope %s took %.02f ms.\n", _scope_name, elapsedTime.count());
}
