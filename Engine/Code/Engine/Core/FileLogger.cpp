#include "Engine/Core/FileLogger.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/ThreadUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Profiling/AllocationTracker.hpp"

#include "Engine/Services/IJobSystemService.hpp"
#include "Engine/Services/ServiceLocator.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <iostream>

namespace FS = std::filesystem;

FileLogger::FileLogger(const std::string& logName) noexcept
: IFileLoggerService()
{
    Initialize(logName);
}

FileLogger::~FileLogger() noexcept {
    Shutdown();
}

void FileLogger::Log_worker() noexcept {
    JobConsumer jc;
    jc.AddCategory(JobType::Logging);
    auto* js = ServiceLocator::get<IJobSystemService, NullJobSystemService>();
    js->SetCategorySignal(JobType::Logging, &m_signal);

    while(IsRunning()) {
        std::unique_lock<std::mutex> lock(m_cs);
        //Condition to wake up: not running or queue has jobs.
        m_signal.wait(lock, [this]() -> bool { return !m_is_running || !m_queue.empty(); });
        if(!m_queue.empty()) {
            const std::string str = m_queue.front();
            m_queue.pop();
            m_stream << str;
            RequestFlush();
            jc.ConsumeAll();
        }
    }
}

void FileLogger::RequestFlush() noexcept {
    if(m_requesting_flush) {
        m_stream.flush();
        m_requesting_flush = false;
    }
}

bool FileLogger::IsRunning() const noexcept {
    bool running = false;
    {
        std::scoped_lock<std::mutex> lock(m_cs);
        running = m_is_running;
    }
    return running;
}

struct copy_log_job_t {
    std::filesystem::path from{};
    std::filesystem::path to{};
};

void FileLogger::DoCopyLog() noexcept {
    if(IsRunning()) {
        auto* job_data = new copy_log_job_t;
        std::filesystem::path from_p = m_current_log_path;
        from_p = FS::canonical(from_p);
        from_p.make_preferred();
        auto to_p = from_p.parent_path();
        to_p = to_p / std::filesystem::path{m_logName + "_" + TimeUtils::GetDateTimeStampFromNow({true})}.replace_extension(".log");
        to_p = FS::absolute(to_p);
        to_p.make_preferred();
        job_data->to = to_p;
        job_data->from = from_p;
        ServiceLocator::get<IJobSystemService, NullJobSystemService>()->Run(
        JobType::Generic, [this](void* user_data) { CopyLog(user_data); }, job_data);
    }
}

void FileLogger::CopyLog(void* user_data) noexcept {
    if(IsRunning()) {
        auto* job_data = static_cast<copy_log_job_t*>(user_data);
        std::filesystem::path from = job_data->from;
        std::filesystem::path to = job_data->to;
        std::scoped_lock<std::mutex> lock(m_cs);
        m_stream.flush();
        m_stream.close();
        std::cout.rdbuf(m_old_cout);
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
        m_stream.open(from, std::ios_base::app);
        m_old_cout = std::cout.rdbuf(m_stream.rdbuf());
    }
}

void FileLogger::FinalizeLog() noexcept {
    std::filesystem::path from_p = m_current_log_path;
    from_p = FS::canonical(from_p);
    from_p.make_preferred();
    std::filesystem::path to_p = from_p;
    auto logname = to_p.filename().stem().string();
    TimeUtils::DateTimeStampOptions opts;
    opts.use_separator = true;
    opts.is_filename = true;
    to_p.replace_filename(logname + "_" + TimeUtils::GetDateTimeStampFromNow(opts));
    to_p.replace_extension(".log");
    //Canonicalizing output file that doesn't already exist is an error.
    to_p.make_preferred();
    m_stream << "Copied log to: " << to_p << "...\n";
    m_stream.flush();
    m_stream.close();
    std::cout.rdbuf(m_old_cout);
    std::filesystem::copy_file(from_p, to_p, std::filesystem::copy_options::overwrite_existing);
}

void FileLogger::Initialize(const std::string& log_name) noexcept {
    if(IsRunning()) {
        LogLine("FileLogger already running.");
        return;
    }
    namespace FS = std::filesystem;
    const auto folder_p = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameLogs);
    const auto extension = FS::path{".log"};
    m_logName = log_name;
    const auto log_p = [&]() {
        FS::path result = (folder_p / log_name).replace_extension(extension);
        if(FS::exists(result)) {
            result = FS::canonical(result);
        } else {
            result = FS::absolute(result);
        }
        result.make_preferred();
        return result;
    }();
    m_current_log_path = log_p;
    FileUtils::CreateFolders(folder_p); //I don't care if this returns false when the folders already exist.
    //Removes only if it exists.
    FS::remove(log_p);
    FileUtils::RemoveExceptMostRecentFiles(folder_p, MAX_LOGS, extension.string());
    m_is_running = true;
    m_stream.open(m_current_log_path);
    if(m_stream.fail()) {
        DebuggerPrintf("FileLogger failed to initialize.\n");
        m_stream.clear();
        m_is_running = false;
        return;
    }
    m_old_cout = std::cout.rdbuf(m_stream.rdbuf());
    m_worker = std::thread(&FileLogger::Log_worker, this);
    ThreadUtils::SetThreadDescription(m_worker, L"FileLogger");
    const auto ss = std::string{"Initializing Logger: "} + m_current_log_path.string() + "...";
    LogLine(ss.c_str());
}

void FileLogger::Shutdown() noexcept {
    if(IsRunning()) {
        {
            auto ss = std::ostringstream{};
            if(AllocationTracker::is_enabled()) {
                ss << AllocationTracker::status() << "\n";
            }
            ss << std::string{"Shutting down Logger: "} << m_current_log_path.string() << "...";
            LogLine(ss.str().c_str());
        }
        SetIsRunning(false);
        m_signal.notify_all();
        if(m_worker.joinable()) {
            m_worker.join();
        }
        FinalizeLog();
        ServiceLocator::get<IJobSystemService, NullJobSystemService>()->SetCategorySignal(JobType::Logging, nullptr);
    }
}

void FileLogger::Log(const std::string& msg) noexcept {
    {
        std::scoped_lock<std::mutex> lock(m_cs);
        m_queue.push(msg);
    }
    m_signal.notify_all();
}

void FileLogger::LogLine(const std::string& msg) noexcept {
    Log(msg + '\n');
}

void FileLogger::LogAndFlush(const std::string& msg) noexcept {
    Log(msg);
    Flush();
}

void FileLogger::LogLineAndFlush(const std::string& msg) noexcept {
    LogLine(msg);
    Flush();
}

void FileLogger::LogPrint(const std::string& msg) noexcept {
    LogTag("log", msg);
}

void FileLogger::LogWarn(const std::string& msg) noexcept {
    LogTag("warning", msg);
}

void FileLogger::LogError(const std::string& msg) noexcept {
    LogTag("error", msg);
}

void FileLogger::LogTag(const std::string& tag, const std::string& msg) noexcept {
    std::stringstream ss;
    InsertTimeStamp(ss);
    InsertTag(ss, tag);
    InsertMessage(ss, msg);

    Log(ss.str());
}

void FileLogger::LogPrintLine(const std::string& msg) noexcept {
    LogTagLine("log", msg);
}

void FileLogger::LogWarnLine(const std::string& msg) noexcept {
    LogTagLine("warning", msg);
}

void FileLogger::LogErrorLine(const std::string& msg) noexcept {
    LogTagLine("error", msg);
}

void FileLogger::LogTagLine(const std::string& tag, const std::string& msg) noexcept {
    LogTag(tag, msg + '\n');
}

void FileLogger::InsertTimeStamp(std::stringstream& msg) noexcept {
    TimeUtils::DateTimeStampOptions opts{};
    opts.use_separator = true;
    msg << "[" << TimeUtils::GetDateTimeStampFromNow(opts) << "]";
}

void FileLogger::InsertTag(std::stringstream& msg, const std::string& tag) noexcept {
    msg << "[" << tag << "]";
}

void FileLogger::InsertMessage(std::stringstream& msg, const std::string& messageLiteral) noexcept {
    msg << ' ' << messageLiteral;
}

void FileLogger::Flush() noexcept {
    m_requesting_flush = true;
    while(m_requesting_flush) {
        std::this_thread::yield();
    }
}

void FileLogger::SetIsRunning(bool value /*= true*/) noexcept {
    std::scoped_lock<std::mutex> lock(m_cs);
    m_is_running = value;
}

void FileLogger::SaveLog() noexcept {
    DoCopyLog();
}
