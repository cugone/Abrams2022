#pragma once

#include "Engine/Core/ThreadSafeQueue.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

class JobSystem;

class FileLogger : public IFileLoggerService {
public:
    FileLogger() = delete;
    explicit FileLogger(const std::string& logName) noexcept;
    FileLogger(const FileLogger& rhs) = delete;
    FileLogger(FileLogger&& rhs) = delete;
    FileLogger& operator=(const FileLogger& rhs) = delete;
    FileLogger& operator=(FileLogger&& rhs) = delete;
    virtual ~FileLogger() noexcept;

    void Shutdown() noexcept;
    void Log(const std::string& msg) noexcept override;
    void LogLine(const std::string& msg) noexcept override;
    void LogAndFlush(const std::string& msg) noexcept override;
    void LogLineAndFlush(const std::string& msg) noexcept override;
    void LogPrint(const std::string& msg) noexcept override;
    void LogWarn(const std::string& msg) noexcept override;
    void LogError(const std::string& msg) noexcept override;
    void LogTag(const std::string& tag, const std::string& msg) noexcept override;
    void LogPrintLine(const std::string& msg) noexcept override;
    void LogWarnLine(const std::string& msg) noexcept override;
    void LogErrorLine(const std::string& msg) noexcept override;
    void LogTagLine(const std::string& tag, const std::string& msg) noexcept override;
    void Flush() noexcept override;
    void SetIsRunning(bool value = true) noexcept;

    void SaveLog() noexcept override;

protected:
private:
    void Initialize(const std::string& log_name) noexcept;

    void InsertTimeStamp(std::stringstream& msg) noexcept;
    void InsertTag(std::stringstream& msg, const std::string& tag) noexcept;
    void InsertMessage(std::stringstream& msg, const std::string& messageLiteral) noexcept;

    void Log_worker() noexcept;
    void RequestFlush() noexcept;
    [[nodiscard]] bool IsRunning() const noexcept;

    void DoCopyLog() noexcept;
    void CopyLog(void* user_data) noexcept;
    void FinalizeLog() noexcept;
    mutable std::mutex _cs{};
    std::ofstream _stream{};
    std::string _logName{};
    std::filesystem::path _current_log_path{};
    std::streambuf* _old_cout{};
    std::thread _worker{};
    std::condition_variable _signal{};
    ThreadSafeQueue<std::string> _queue;
    std::atomic_bool _is_running = false;
    std::atomic_bool _requesting_flush = false;
};
