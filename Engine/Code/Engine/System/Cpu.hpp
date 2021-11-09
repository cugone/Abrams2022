#pragma once

#include <ostream>

namespace System::Cpu {
// clang-format off
enum class ProcessorArchitecture {
    Intel
    , Mips
    , Alpha
    , Ppc
    , Shx
    , Arm
    , Ia64
    , Alpha64
    , Msil
    , Amd64
    , Ia32OnWin64
    , Neutral
    , Arm64
    , Arm32OnWin64
    , Ia32OnArm64
    , x64 = Amd64
    , x86 = Intel
    , Unknown = 0xFFFF
};
// clang-format on
struct CpuDesc {
    ProcessorArchitecture type{};
    unsigned long socketCount = 0;
    unsigned long logicalCount = 0;
};
std::ostream& operator<<(std::ostream& out, const CpuDesc& cpu) noexcept;

[[nodiscard]] CpuDesc GetCpuDesc() noexcept;

} // namespace System::Cpu

namespace StringUtils {
[[nodiscard]] std::string to_string(const System::Cpu::ProcessorArchitecture& architecture) noexcept;
}
