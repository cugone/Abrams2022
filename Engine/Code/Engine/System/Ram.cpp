#include "Engine/System/Ram.hpp"

#include "Engine/Platform/Win.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <iomanip>

unsigned long long GetPhysicalRam() noexcept;
unsigned long long GetAvailableRam() noexcept;

std::ostream& System::Ram::operator<<(std::ostream& out, const System::Ram::RamDesc& desc) noexcept {
    const auto entry_name_field_width = std::size_t{40u};
    const auto entry_field_width = std::size_t{35u};
    const auto old_fmt = out.flags();
    const auto old_w = out.width();
    out << std::left << std::setw(entry_name_field_width) << "Installed RAM:" << std::right << std::setw(entry_field_width) << std::fixed << std::setprecision(1) << static_cast<long double>(desc.installed) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den << " GB\n";
    out << std::left << std::setw(entry_name_field_width) << "Available RAM:" << std::right << std::setw(entry_field_width) << std::fixed << std::setprecision(1) << static_cast<long double>(desc.available) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den << " GB\n";
    out.flags(old_fmt);
    out.width(old_w);
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
