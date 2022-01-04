#pragma once

#include <string>
#include <thread>

namespace ThreadUtils {
void SetThreadDescription(std::thread& thread, const std::string& description) noexcept;
void SetThreadDescription(std::thread& thread, const std::wstring& description) noexcept;
void GetThreadDescription(std::thread& thread, std::string& description) noexcept;
void GetThreadDescription(std::thread& thread, std::wstring& description) noexcept;
} // namespace ThreadUtils
