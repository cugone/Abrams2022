#pragma once

#include "Engine/Services/IService.hpp"

#include <string>

class IFileLoggerService : public IService {
public:
    virtual ~IFileLoggerService() noexcept {/* DO NOTHING */};

    virtual void Log(const std::string& msg) noexcept = 0;
    virtual void LogLine(const std::string& msg) noexcept = 0;
    virtual void LogAndFlush(const std::string& msg) noexcept = 0;
    virtual void LogLineAndFlush(const std::string& msg) noexcept = 0;
    virtual void LogPrint(const std::string& msg) noexcept = 0;
    virtual void LogWarn(const std::string& msg) noexcept = 0;
    virtual void LogError(const std::string& msg) noexcept = 0;
    virtual void LogTag(const std::string& tag, const std::string& msg) noexcept = 0;
    virtual void LogPrintLine(const std::string& msg) noexcept = 0;
    virtual void LogWarnLine(const std::string& msg) noexcept = 0;
    virtual void LogErrorLine(const std::string& msg) noexcept = 0;
    virtual void LogTagLine(const std::string& tag, const std::string& msg) noexcept = 0;
    virtual void Flush() noexcept = 0;
    virtual void SaveLog() noexcept = 0;

protected:
private:
};

class NullFileLoggerService : public IFileLoggerService {
public:
    void Log([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogLine([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogAndFlush([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogLineAndFlush([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogPrint([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogWarn([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogError([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogTag([[maybe_unused]] const std::string& tag, [[maybe_unused]] const std::string& msg) noexcept override {}
    void LogPrintLine([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogWarnLine([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogErrorLine([[maybe_unused]] const std::string& msg) noexcept override {}
    void LogTagLine([[maybe_unused]] const std::string& tag, [[maybe_unused]] const std::string& msg) noexcept override {}
    void Flush() noexcept override {}
    void SaveLog() noexcept override {}
};
