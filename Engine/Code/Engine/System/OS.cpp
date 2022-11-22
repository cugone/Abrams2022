#include "Engine/System/OS.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include <format>
#include <sstream>

#ifdef PLATFORM_WINDOWS
    #define HAS_VERSION_HELPERS
    #include "Engine/Platform/Win.hpp"

    #include <VersionHelpers.h>
    #include <processthreadsapi.h>
    #include <wow64apiset.h>
#endif

std::ostream& System::OS::operator<<(std::ostream& out, const System::OS::OsDesc& os) noexcept {
    out << std::format("{:<40}{:>35}\n", "Operating System:", os.VersionFriendly);
    return out;
}

System::OS::OsDesc System::OS::GetOsDesc() noexcept {
    OsDesc desc{};
    const auto& type = GetOperatingSystemType();
    desc.type = type;
    desc.VersionFriendly = GetFriendlyStringFromOperatingSystemType(type);
    return desc;
}

System::OS::OperatingSystem& System::OS::operator&=(OperatingSystem& a, const OperatingSystem& b) noexcept {
    using underlying = std::underlying_type_t<OperatingSystem>;
    const auto underlying_a = static_cast<underlying>(a);
    const auto underlying_b = static_cast<underlying>(b);
    a = static_cast<OperatingSystem>(underlying_a & underlying_b);
    return a;
}

System::OS::OperatingSystem System::OS::operator&(OperatingSystem a, const OperatingSystem& b) noexcept {
    a &= b;
    return a;
}

System::OS::OperatingSystem& System::OS::operator|=(OperatingSystem& a, const OperatingSystem& b) noexcept {
    using underlying = std::underlying_type_t<OperatingSystem>;
    const auto underlying_a = static_cast<underlying>(a);
    const auto underlying_b = static_cast<underlying>(b);
    a = static_cast<OperatingSystem>(underlying_a | underlying_b);
    return a;
}

System::OS::OperatingSystem System::OS::operator|(OperatingSystem a, const OperatingSystem& b) noexcept {
    a |= b;
    return a;
}

System::OS::OperatingSystem System::OS::operator~(const OperatingSystem& a) noexcept {
    using underlying = std::underlying_type_t<OperatingSystem>;
    const auto underlying_a = static_cast<underlying>(a);
    return static_cast<OperatingSystem>(~underlying_a);
}

System::OS::OperatingSystemArchitecture System::OS::GetOperatingSystemArchitecture() noexcept {
    OperatingSystemArchitecture arch{OperatingSystemArchitecture::Unknown};
#ifdef HAS_VERSION_HELPERS
    const auto pid = ::GetCurrentProcess();
    USHORT process_machine_raw{};
    USHORT native_machine_raw{};
    const auto succeeded = !!::IsWow64Process2(pid, &process_machine_raw, &native_machine_raw);
    if(!succeeded) {
        return arch;
    }
    if(process_machine_raw != IMAGE_FILE_MACHINE_UNKNOWN) {
        arch = OperatingSystemArchitecture::x64;
    } else {
        arch = OperatingSystemArchitecture::x86;
    }
#endif
    return arch;
}

System::OS::OperatingSystem System::OS::GetOperatingSystemType() noexcept {
    OperatingSystem type{OperatingSystem::Unknown};
#ifdef HAS_VERSION_HELPERS
    const auto pid = ::GetCurrentProcess();
    USHORT process_machine_raw{};
    USHORT native_machine_raw{};
    const auto succeeded = !!::IsWow64Process2(pid, &process_machine_raw, &native_machine_raw);
    if(!succeeded) {
        return type;
    }
    if(process_machine_raw != IMAGE_FILE_MACHINE_UNKNOWN) {
        type |= (OperatingSystem::Windows | OperatingSystem::Windows_x86);
    } else {
        type |= (OperatingSystem::Windows | OperatingSystem::Windows_x64);
    }

    if(::IsWindowsServer()) {
        type |= OperatingSystem::Windows_Server;
    }
    if(::IsWindows10OrGreater()) {
        type |= OperatingSystem::Windows_10;
    } else if(::IsWindows8Point1OrGreater()) {
        type |= OperatingSystem::Windows_8Point1;
    } else if(::IsWindows8OrGreater()) {
        type |= OperatingSystem::Windows_8;
    } else if(::IsWindows7SP1OrGreater()) {
        type |= OperatingSystem::Windows_7Sp1;
    } else if(::IsWindows7OrGreater()) {
        type |= OperatingSystem::Windows_7;
    } else if(::IsWindowsVistaSP2OrGreater()) {
        type |= OperatingSystem::Windows_VistaSp2;
    } else if(::IsWindowsVistaSP1OrGreater()) {
        type |= OperatingSystem::Windows_VistaSp1;
    } else if(::IsWindowsVistaOrGreater()) {
        type |= OperatingSystem::Windows_Vista;
    } else if(::IsWindowsXPSP3OrGreater()) {
        type |= OperatingSystem::Windows_XpSp3;
    } else if(::IsWindowsXPSP2OrGreater()) {
        type |= OperatingSystem::Windows_XpSp2;
    } else if(::IsWindowsXPSP1OrGreater()) {
        type |= OperatingSystem::Windows_XpSp1;
    } else if(::IsWindowsXPOrGreater()) {
        type |= OperatingSystem::Windows_Xp;
    } else {
        /* DO NOTHING */
    }
#endif
    return type;
}

std::string System::OS::GetFriendlyStringFromOperatingSystemType(System::OS::OperatingSystem type) noexcept {
    using namespace std::literals::string_literals;
    if((type & OperatingSystem::Windows) != OperatingSystem::Unknown) {
        if((type & OperatingSystem::Windows_x86) != OperatingSystem::Unknown) {
            return "Windows 32-bit"s;
        }
        if((type & OperatingSystem::Windows_x64) != OperatingSystem::Unknown) {
            return "Windows 64-bit"s;
        }
    }
    return {};
}
