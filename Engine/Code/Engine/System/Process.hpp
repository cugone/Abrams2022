#pragma once

#include <string>
#include <vector>

namespace System {

class Process {
public:

    Process() = delete;
    Process(const Process& other) = delete;
    Process(Process&& other) noexcept;
    Process& operator=(const Process& other) = delete;
    Process& operator=(Process&& other) noexcept;
    ~Process();

    explicit Process(std::uint32_t pid);

    [[nodiscard]] static std::vector<uint32_t> GetPids() noexcept;
    [[nodiscard]] static std::vector<Process> Find(std::string_view name);
    [[nodiscard]] static Process Find(std::uint32_t pid) noexcept;
    [[nodiscard]] std::string GetName() const;
    [[nodiscard]] std::uint32_t GetPid() const noexcept;

protected:
private:
    mutable std::string m_name;
    void* m_handle;
    std::uint32_t m_pid;
};

} // namespace System
