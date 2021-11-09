#pragma once

#include <string>
#include <thread>

struct joinable_thread : public std::thread {
    ~joinable_thread() {
        if(joinable()) {
            join();
        }
    }
    void detach() = delete;
};

using jthread = joinable_thread;

namespace ThreadUtils {
void SetThreadDescription(std::thread& thread, const std::string& description) noexcept;
void SetThreadDescription(std::thread& thread, const std::wstring& description) noexcept;
void GetThreadDescription(std::thread& thread, std::string& description) noexcept;
void GetThreadDescription(std::thread& thread, std::wstring& description) noexcept;
} // namespace ThreadUtils
