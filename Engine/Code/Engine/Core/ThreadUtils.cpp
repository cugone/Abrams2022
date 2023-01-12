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

unsigned long GetProcessId() noexcept {
#ifdef PLATFORM_WINDOWS
    return static_cast<unsigned long>(::GetCurrentProcessId());
#else
    return 0ul;
#endif
}

unsigned long GetProcessIDFromThread(std::jthread& thread) noexcept {
#ifdef PLATFORM_WINDOWS
    return static_cast<unsigned long>(::GetProcessIdOfThread(thread.native_handle()));
#else
    return 0ul;
#endif
}

unsigned long GetProcessIDFromThisThread() noexcept {
#ifdef PLATFORM_WINDOWS
    return static_cast<unsigned long>(::GetProcessIdOfThread(::GetCurrentThread()));
#else
    return 0ul;
#endif
}

void SetThreadDescription(std::jthread& thread, const std::string& description) noexcept {
    auto wide_description = StringUtils::ConvertMultiByteToUnicode(description);
    SetThreadDescription(thread, wide_description);
}

void SetThreadDescription(std::jthread& thread, const std::wstring& description) noexcept {
#ifdef PLATFORM_WINDOWS
    ::SetThreadDescription(thread.native_handle(), description.c_str());
#endif
}

void GetThreadDescription(std::jthread& thread, std::string& description) noexcept {
    std::wstring wide_description{};
    GetThreadDescription(thread, wide_description);
    description = StringUtils::ConvertUnicodeToMultiByte(wide_description);
}

void GetThreadDescription(std::jthread& thread, std::wstring& description) noexcept {
#ifdef PLATFORM_WINDOWS
    PWSTR d{};
    ::GetThreadDescription(thread.native_handle(), &d);
    description.assign(d);
    ::LocalFree(d);
    d = nullptr;
#endif
}

} // namespace ThreadUtils