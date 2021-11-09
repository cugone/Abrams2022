#pragma once

#include "Engine/Services/IService.hpp"

#include <filesystem>
#include <string>

struct ConsoleCommand { virtual ~ConsoleCommand() noexcept = default; };
struct ConsoleCommandList { virtual ~ConsoleCommandList() noexcept = default; };

class IConsoleService : public IService {
public:
    virtual ~IConsoleService() noexcept {};

    virtual void RunCommand([[maybe_unused]] const std::string& name_and_args) noexcept = 0;
    virtual void RegisterCommand([[maybe_unused]] const ConsoleCommand& command) noexcept = 0;
    virtual void UnregisterCommand([[maybe_unused]] const std::string& command_name) noexcept = 0;
    virtual void PushCommandList([[maybe_unused]] const ConsoleCommandList& list) noexcept = 0;
    virtual void PopCommandList([[maybe_unused]] const ConsoleCommandList& list) noexcept = 0;
    virtual void PrintMsg([[maybe_unused]] const std::string& msg) noexcept = 0;
    virtual void WarnMsg([[maybe_unused]] const std::string& msg) noexcept = 0;
    virtual void ErrorMsg([[maybe_unused]] const std::string& msg) noexcept = 0;

    [[nodiscard]] virtual void* GetAcceleratorTable() const noexcept = 0;
    [[nodiscard]] virtual bool IsOpen() const noexcept = 0;
    [[nodiscard]] virtual bool IsClosed() const noexcept = 0;

protected:
private:
    
};

class NullConsoleService : public IConsoleService {
public:
    ~NullConsoleService() noexcept override {};
    void RunCommand([[maybe_unused]] const std::string& name_and_args) noexcept override {};
    void RegisterCommand([[maybe_unused]] const ConsoleCommand& command) noexcept override {};
    void UnregisterCommand([[maybe_unused]] const std::string& command_name) noexcept override {};
    void PushCommandList([[maybe_unused]] const ConsoleCommandList& list) noexcept override {};
    void PopCommandList([[maybe_unused]] const ConsoleCommandList& list) noexcept override {};
    void PrintMsg([[maybe_unused]] const std::string& msg) noexcept override {};
    void WarnMsg([[maybe_unused]] const std::string& msg) noexcept override {};
    void ErrorMsg([[maybe_unused]] const std::string& msg) noexcept override {};

    [[nodiscard]] virtual void* GetAcceleratorTable() const noexcept override {};
    [[nodiscard]] virtual bool IsOpen() const noexcept override { return false; };
    [[nodiscard]] virtual bool IsClosed() const noexcept override { return true; };

protected:
private:
    
};
