#include "Engine/Core/ThreadUtils.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"

namespace ThreadUtils {

void SetThreadDescription(std::thread& thread, const std::string& description) noexcept {
    auto wide_description = StringUtils::ConvertMultiByteToUnicode(description);
    SetThreadDescription(thread, wide_description);
}

void SetThreadDescription(std::thread& thread, const std::wstring& description) noexcept {
#ifdef PLATFORM_WINDOWS
    ::SetThreadDescription(thread.native_handle(), description.c_str());
#endif
}

void GetThreadDescription(std::thread& thread, std::string& description) noexcept {
    std::wstring wide_description{};
    GetThreadDescription(thread, wide_description);
    description = StringUtils::ConvertUnicodeToMultiByte(wide_description);
}

void GetThreadDescription(std::thread& thread, std::wstring& description) noexcept {
#ifdef PLATFORM_WINDOWS
    PWSTR d{};
    ::GetThreadDescription(thread.native_handle(), &d);
    description.assign(d);
    ::LocalFree(d);
    d = nullptr;
#endif
}

} // namespace ThreadUtils