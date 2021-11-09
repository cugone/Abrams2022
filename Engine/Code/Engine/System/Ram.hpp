#pragma once

#include <ostream>

namespace System::Ram {

struct RamDesc {
    unsigned long long installed{};
    unsigned long long available{};
};
std::ostream& operator<<(std::ostream& out, const RamDesc& desc) noexcept;

[[nodiscard]] RamDesc GetRamDesc() noexcept;

} // namespace System::Ram