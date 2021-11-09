#include "Engine/System/System.hpp"

#include <sstream>

std::string StringUtils::to_string(const System::SystemDesc& system) noexcept {
    std::stringstream ss;
    ss << system.os;
    ss << system.cpu;
    ss << system.ram;
    return ss.str();
}

std::ostream& System::operator<<(std::ostream& out, const System::SystemDesc& desc) noexcept {
    const auto old_fmt = out.flags();
    const auto old_w = out.width();
    out << "SYSTEM:\n";
    out << desc.os;
    out << desc.cpu;
    out << desc.ram;
    out << '\n';
    out.flags(old_fmt);
    out.width(old_w);
    return out;
}

System::SystemDesc System::GetSystemDesc() noexcept {
    SystemDesc desc{};
    desc.cpu = Cpu::GetCpuDesc();
    desc.ram = Ram::GetRamDesc();
    desc.os = OS::GetOsDesc();
    return desc;
}
