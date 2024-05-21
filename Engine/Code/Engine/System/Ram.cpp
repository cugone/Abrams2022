#include "Engine/System/Ram.hpp"

#include "Engine/Platform/Win.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <format>

unsigned long long GetPhysicalRam() noexcept;
unsigned long long GetAvailableRam() noexcept;

std::ostream& System::Ram::operator<<(std::ostream& out, const System::Ram::RamDesc& desc) noexcept {
    const auto installed = static_cast<long double>(desc.installed) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den;
    const auto available = static_cast<long double>(desc.available) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den;
    out << std::vformat("{:<40}{:>35.1f} GB\n", std::make_format_args("Installed RAM:", installed));
    out << std::vformat("{:<40}{:>35.1f} GB\n", std::make_format_args("Available RAM:", available));
    return out;
}

System::Ram::RamDesc System::Ram::GetRamDesc() noexcept {
    RamDesc desc{};
    desc.available = GetAvailableRam();
    desc.installed = GetPhysicalRam();
    return desc;
}

unsigned long long GetPhysicalRam() noexcept {
    uint64_t pram = 0;
    ::GetPhysicallyInstalledSystemMemory(&pram);
    return static_cast<unsigned long long>(pram * MathUtils::BYTES_KIB_RATIO.num / MathUtils::BYTES_KIB_RATIO.den);
}

unsigned long long GetAvailableRam() noexcept {
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    ::GlobalMemoryStatusEx(&mem);
    return mem.ullTotalPhys;
}
