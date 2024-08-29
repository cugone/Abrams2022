#include "Engine/System/Cpu.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/System/OS.hpp"

#include <format>
#include <memory>
#include <sstream>

std::string StringUtils::to_string(const System::Cpu::ProcessorArchitecture& architecture) noexcept {
    switch(architecture) {
    case System::Cpu::ProcessorArchitecture::Unknown: return "Unknown";
    case System::Cpu::ProcessorArchitecture::x64: return "x64"; //Also Amd64
    case System::Cpu::ProcessorArchitecture::Arm: return "ARM";
    case System::Cpu::ProcessorArchitecture::Arm64: return "ARM 64";
    case System::Cpu::ProcessorArchitecture::Ia64: return "Intel Itanium 64";
    case System::Cpu::ProcessorArchitecture::x86: return "x86"; //Also Intel
    case System::Cpu::ProcessorArchitecture::Mips: return "Mips";
    case System::Cpu::ProcessorArchitecture::Alpha: return "Alpha";
    case System::Cpu::ProcessorArchitecture::Ppc: return "PPC";
    case System::Cpu::ProcessorArchitecture::Shx: return "SHX";
    case System::Cpu::ProcessorArchitecture::Alpha64: return "Alpha 64";
    case System::Cpu::ProcessorArchitecture::Msil: return "MSIL";
    case System::Cpu::ProcessorArchitecture::Ia32OnWin64: return "Intel Itanium on Win64";
    case System::Cpu::ProcessorArchitecture::Neutral: return "Neutral";
    case System::Cpu::ProcessorArchitecture::Arm32OnWin64: return "ARM32 on Win64";
    case System::Cpu::ProcessorArchitecture::Ia32OnArm64: return "Intel Itanium on ARM64";
    default: return "";
    }
}

System::Cpu::ProcessorArchitecture GetProcessorArchitecture() noexcept;
unsigned long GetLogicalProcessorCount() noexcept;
unsigned long GetSocketCount() noexcept;
SYSTEM_INFO GetSystemInfo() noexcept;

System::Cpu::CpuDesc System::Cpu::GetCpuDesc() noexcept {
    CpuDesc desc{};
    desc.type = GetProcessorArchitecture();
    desc.logicalCount = GetLogicalProcessorCount();
    desc.socketCount = GetSocketCount();
    return desc;
}

std::ostream& System::Cpu::operator<<(std::ostream& out, const System::Cpu::CpuDesc& cpu) noexcept {
    out << std::format("{:<40}{:>35}\n", "Processor Type:",  StringUtils::to_string(cpu.type));
    out << std::format("{:<40}{:>35}\n", "Socket Count:", cpu.socketCount);
    out << std::format("{:<40}{:>35}\n", "Logical Processor Count:", cpu.logicalCount);
    return out;
}

SYSTEM_INFO GetSystemInfo() noexcept {
    SYSTEM_INFO info{};
    switch(System::OS::GetOperatingSystemArchitecture()) {
    case System::OS::OperatingSystemArchitecture::x86: {
        ::GetSystemInfo(&info);
        return info;
    }
    case System::OS::OperatingSystemArchitecture::x64: {
        ::GetNativeSystemInfo(&info);
        return info;
    }
    case System::OS::OperatingSystemArchitecture::Unknown:
    default: {
        return info;
    }
    }
}

System::Cpu::ProcessorArchitecture GetProcessorArchitecture() noexcept {
    const auto info = GetSystemInfo();
    switch(info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL: return System::Cpu::ProcessorArchitecture::Intel;
    case PROCESSOR_ARCHITECTURE_MIPS: return System::Cpu::ProcessorArchitecture::Mips;
    case PROCESSOR_ARCHITECTURE_ALPHA: return System::Cpu::ProcessorArchitecture::Alpha;
    case PROCESSOR_ARCHITECTURE_PPC: return System::Cpu::ProcessorArchitecture::Ppc;
    case PROCESSOR_ARCHITECTURE_SHX: return System::Cpu::ProcessorArchitecture::Shx;
    case PROCESSOR_ARCHITECTURE_ARM: return System::Cpu::ProcessorArchitecture::Arm;
    case PROCESSOR_ARCHITECTURE_IA64: return System::Cpu::ProcessorArchitecture::Ia64;
    case PROCESSOR_ARCHITECTURE_ALPHA64: return System::Cpu::ProcessorArchitecture::Alpha64;
    case PROCESSOR_ARCHITECTURE_MSIL: return System::Cpu::ProcessorArchitecture::Msil;
    case PROCESSOR_ARCHITECTURE_AMD64: return System::Cpu::ProcessorArchitecture::Amd64;
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64: return System::Cpu::ProcessorArchitecture::Ia32OnWin64;
    case PROCESSOR_ARCHITECTURE_NEUTRAL: return System::Cpu::ProcessorArchitecture::Neutral;
    case PROCESSOR_ARCHITECTURE_ARM64: return System::Cpu::ProcessorArchitecture::Arm64;
    case PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64: return System::Cpu::ProcessorArchitecture::Arm32OnWin64;
    case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64: return System::Cpu::ProcessorArchitecture::Ia32OnArm64;
    case PROCESSOR_ARCHITECTURE_UNKNOWN: return System::Cpu::ProcessorArchitecture::Unknown;
    default: return System::Cpu::ProcessorArchitecture::Unknown;
    }
}

unsigned long GetLogicalProcessorCount() noexcept {
    SYSTEM_INFO info = GetSystemInfo();
    return info.dwNumberOfProcessors;
}

unsigned long GetSocketCount() noexcept {
    DWORD length{};
    unsigned long socketCount{};
    //This will intentionally fail in order to fill the length parameter with the correct value.
    if(!::GetLogicalProcessorInformation(nullptr, &length)) {
        if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            const auto b = std::make_unique<unsigned char[]>(length);
            auto* buffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(b.get());
            if(!::GetLogicalProcessorInformation(buffer, &length)) {
                return 0ul;
            }
            std::stringstream ss(std::ios_base::binary | std::ios_base::in | std::ios_base::out);
            if(!ss.write(reinterpret_cast<const char*>(buffer), length)) {
                return 0ul;
            }
            ss.clear();
            ss.seekg(0);
            ss.seekp(0);
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION p{};
            while(ss.read(reinterpret_cast<char*>(&p), sizeof(p))) {
                switch(p.Relationship) {
                case RelationProcessorPackage: {
                    ++socketCount;
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
    return socketCount;
}
