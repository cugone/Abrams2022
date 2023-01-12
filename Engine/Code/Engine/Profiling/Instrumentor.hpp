//
// Basic instrumentation profiler by Cherno

// Usage: include this header file somewhere in your code (eg. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
// You will probably want to macro-fy this, to switch on/off easily and use things like __FUNCSIG__ for the profile name.
//
#pragma once

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ThreadUtils.hpp"

#include "Engine/Platform/Win.hpp"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <source_location>

struct ProfileResult {
    std::string Name;
    long long Start;
    long long End;
    std::jthread::id ThreadID;
};

struct ProfileMetadata {
    long long ProcessID{};
    long long processSortIndex{};
    long long threadSortIndex{};
    std::jthread::id threadID{};
    std::string processName{};
    std::string processLabels{};
    std::string threadName{};
};

enum class MetaDataCategory {
    ProcessName
    ,ProcessLabels
    ,ProcessSortIndex
    ,ThreadName
    ,ThreadSortIndex
};

struct InstrumentationSession {
    std::string Name;
};

class Instrumentor {
public:
    Instrumentor() = default;
    ~Instrumentor() {
        EndSession();
    }

    void BeginSession(const std::string& name, const std::filesystem::path& filepath = "results.json") {
        m_OutputStream.open(filepath);
        if(m_OutputStream.is_open()) {
            WriteHeader();
            m_CurrentSession = std::make_unique<InstrumentationSession>(name);
        }
    }

    void EndSession() {
        if(m_OutputStream.is_open()) {
            WriteFooter();
            m_OutputStream.close();
            m_CurrentSession.reset();
            m_ProfileCount = 0;
        }
    }

    void WriteSessionData(MetaDataCategory cat, const ProfileMetadata& data) {
        if(!m_OutputStream.is_open()) {
            return;
        }
        std::scoped_lock lock(m_cs);
        if(m_ProfileCount++ > 0) {
            m_OutputStream << ",";
        }
        switch(cat) {
        case MetaDataCategory::ProcessName:
        {
            constexpr auto fmt = R"({{"name": "process_name", "ph": "M", "pid": {}, "tid": {}, "args": {{ "name": "{}" }} }})";
            const auto tid = [&data]() -> unsigned int { std::ostringstream ss; ss << data.threadID; return static_cast<unsigned int>(std::stoull(ss.str())); }();
            m_OutputStream << std::vformat(fmt, std::make_format_args(data.ProcessID, tid, data.processName.c_str()));
            break;
        }
        case MetaDataCategory::ProcessLabels:
            break;
        case MetaDataCategory::ProcessSortIndex:
        {
            constexpr auto fmt = R"({{"name": "process_sort_index", "ph": "M", "pid": {}, "tid": {}, "args": {{ "sort_index": "{}" }} }})";
            const auto tid = [&data]() -> unsigned int { std::ostringstream ss; ss << data.threadID; return static_cast<unsigned int>(std::stoull(ss.str())); }();
            m_OutputStream << std::vformat(fmt, std::make_format_args(data.ProcessID, tid, data.processSortIndex));
            break;
        }
        case MetaDataCategory::ThreadName:
        {
            constexpr auto fmt = R"({{"name": "thread_name", "ph": "M", "pid": {}, "tid": {}, "args": {{ "name": "{}" }} }})";
            const auto tid = [&data]() -> unsigned int { std::ostringstream ss; ss << data.threadID; return static_cast<unsigned int>(std::stoull(ss.str())); }();
            m_OutputStream << std::vformat(fmt, std::make_format_args(data.ProcessID, tid, data.threadName.c_str()));
            break;
        }
        case MetaDataCategory::ThreadSortIndex: {
            constexpr auto fmt = R"({{"name": "thread_sort_index", "ph": "M", "pid": {}, "tid": {}, "args": {{ "sort_index": "{}" }} }})";
            const auto tid = [&data]() -> unsigned int { std::ostringstream ss; ss << data.threadID; return static_cast<unsigned int>(std::stoull(ss.str())); }();
            m_OutputStream << std::vformat(fmt, std::make_format_args(data.ProcessID, tid, data.threadSortIndex));
            break;
        }
        default:
            break;
        }
    }

    void WriteProfile(const ProfileResult& result) {
        if(!m_OutputStream.is_open()) {
            return;
        }
        std::scoped_lock lock(m_cs);
        if(m_ProfileCount++ > 0)
            m_OutputStream << ",";

        std::string name = result.Name;
        const auto tid = [result]() -> unsigned int { std::ostringstream ss; ss << result.ThreadID; return static_cast<unsigned int>(std::stoull(ss.str())); }();
        std::replace(std::begin(name), std::end(name), '"', '\'');
        constexpr auto fmt = R"({{"cat":"function","dur":{},"name":"{:s}","ph":"X","pid": {},"tid":{},"ts":{}}})";
        m_OutputStream << std::vformat(fmt, std::make_format_args(
                                            (result.End - result.Start),
                                            name.c_str(),
                                            ThreadUtils::GetProcessId(),
                                            tid,
                                            result.Start));
    }

    void WriteHeader() {
        std::scoped_lock lock(m_cs);
        m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
    }

    void WriteFooter() {
        std::scoped_lock lock(m_cs);
        m_OutputStream << "]}";
    }

    static Instrumentor& Get() {
        static Instrumentor instance;
        return instance;
    }

private:
    std::unique_ptr<InstrumentationSession> m_CurrentSession{nullptr};
    std::ofstream m_OutputStream{};
    mutable std::mutex m_cs{};
    int m_ProfileCount{0};
};

class InstrumentationTimer {
public:
    InstrumentationTimer(const char* name, std::source_location location = std::source_location::current())
    : m_Name(name == nullptr ? location.function_name() : name)
    , m_StartTimepoint(std::chrono::steady_clock::now())
    {
        /* DO NOTHING */
    }

    ~InstrumentationTimer() {
        Stop();
    }

    void Stop() {
        auto endTimepoint = std::chrono::steady_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        Instrumentor::Get().WriteProfile({m_Name, start, end, std::this_thread::get_id()});
    }

private:
    const char* m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
};

#ifdef PROFILE_BUILD

#define PROFILE_BENCHMARK_BEGIN(name, filepath) Instrumentor::Get().BeginSession(name, filepath)
#define PROFILE_BENCHMARK_END() Instrumentor::Get().EndSession()
#define PROFILE_BENCHMARK_SCOPE(name) InstrumentationTimer TOKEN_PASTE(timer,__LINE__)(name)
#define PROFILE_BENCHMARK_FUNCTION() PROFILE_BENCHMARK_SCOPE(nullptr)

#else

#define PROFILE_BENCHMARK_BEGIN(name)
#define PROFILE_BENCHMARK_END()
#define PROFILE_BENCHMARK_SCOPE(name)
#define PROFILE_BENCHMARK_FUNCTION()

#endif
