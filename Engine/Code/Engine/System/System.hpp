#pragma once

#include "Engine/System/Cpu.hpp"
#include "Engine/System/OS.hpp"
#include "Engine/System/Ram.hpp"

#include <ostream>
#include <string>

namespace System {

struct SystemDesc {
    System::Ram::RamDesc ram{};
    System::Cpu::CpuDesc cpu{};
    System::OS::OsDesc os{};
};

std::ostream& operator<<(std::ostream& out, const SystemDesc& desc) noexcept;

[[nodiscard]] SystemDesc GetSystemDesc() noexcept;

} // namespace System

namespace StringUtils {
[[nodiscard]] std::string to_string(const System::SystemDesc& system) noexcept;
}
