#pragma once

#include <string>

class FileDialogs {
public:
    static std::string OpenFile(const char* filter) noexcept;
    static std::string SaveFile(const char* filter) noexcept;
};
