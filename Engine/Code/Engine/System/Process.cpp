#include "Engine/System/Process.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Platform/Win.hpp"

#include "Engine/Core/StringUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <algorithm>
#include <cstdint>
#include <format>

/* static */
std::vector<uint32_t> System::Process::GetPids() noexcept {
#ifdef PLATFORM_WINDOWS
    std::vector<DWORD> pids(1024);
    DWORD bytes_returned{0};
    do {
        pids.resize(pids.size() * 2);
        if(::EnumProcesses(pids.data(), static_cast<DWORD>(pids.size() * sizeof(DWORD)), &bytes_returned) == 0) {
            if(auto* logger = ServiceLocator::get<IFileLoggerService>(); logger != nullptr) {
                logger->LogAndFlush(std::format("Could not enumerate PIDs: {}", StringUtils::FormatWindowsLastErrorMessage()));
                return {};
            }
        }
    } while(bytes_returned == pids.size() / sizeof(DWORD));

    std::vector<uint32_t> returned_pids;
    returned_pids.reserve(pids.size());
    std::copy_if(std::begin(pids), std::end(pids), std::back_inserter(returned_pids), [](DWORD pid) -> std::uint32_t {
        return pid != 0;
    });
    returned_pids.shrink_to_fit();
    return returned_pids;
#else
    return {};
#endif
}

/* static */
std::vector<System::Process> System::Process::Find(std::string_view name) {
    std::vector<Process> procs{};

    const auto pids = GetPids();
    for(const auto& pid : pids) {
        try {
            Process proc{pid};
            if(proc.GetName() == name) {
                procs.emplace_back(std::move(proc));
            }
        } catch(...) {
            /* DO NOTHING */
        }
    }

    return procs;
}

/* static */
System::Process System::Process::Find(std::uint32_t pid) noexcept {
    const auto pids = GetPids();
    if(const auto found = std::find(std::cbegin(pids), std::cend(pids), pid); found != std::cend(pids)) {
        return Process(*found);
    }
    return Process{0};
}

System::Process::Process(std::uint32_t pid)
: m_name()
#ifdef PLATFORM_WINDOWS
, m_handle(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid))
#else
, m_handle(nullptr)
#endif
, m_pid(m_handle ? pid : 0)
{
    if(!m_handle) {
        const auto err = std::format("Process creation failed for PID {} with: {}", pid, StringUtils::FormatWindowsLastErrorMessage());
        throw std::runtime_error(err);
    }
}

System::Process& System::Process::operator=(System::Process&& other) noexcept {
    System::Process new_process(std::move(other));
    
    m_pid = std::move(new_process.m_pid);
    m_name = std::move(new_process.m_name);
    m_handle = std::move(new_process.m_handle);

    return *this;

}

System::Process::Process(Process&& other) noexcept {
    m_pid = std::move(other.m_pid);
    m_name = std::move(other.m_name);
    m_handle = std::move(other.m_handle);

    other.m_pid = 0;
    other.m_name.clear();
    other.m_handle = nullptr;
}

 
System::Process::~Process() {
#ifdef PLATFORM_WINDOWS
    ::CloseHandle(m_handle);
#endif
}

std::string System::Process::GetName() const {
    if(!m_name.empty()) {
        return m_name;
    } else {
#ifdef PLATFORM_WINDOWS
        HMODULE module{};
        DWORD bytes_needed = 0;

        if(::EnumProcessModules(reinterpret_cast<HANDLE>(m_handle), &module, sizeof(module), &bytes_needed)) {
            std::string module_name(MAX_PATH, '\0');
            if(const auto chars_written = ::K32GetModuleBaseNameA(m_handle, module, module_name.data(), static_cast<DWORD>(module_name.size())); chars_written > 0) {
                m_name = std::string(module_name, 0, chars_written);
                return m_name;
            }
        }
#endif
        return {};
    }
}

std::uint32_t System::Process::GetPid() const noexcept {
    return m_pid;
}
