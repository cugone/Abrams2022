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

#include "Engine/Platform/Win.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

struct ProfileResult {
    std::string Name;
    long long Start;
    long long End;
    std::thread::id ThreadID;
};

struct ProfileMetadata {
    long long ProcessID{};
    long long processSortIndex{};
    long long threadSortIndex{};
    std::thread::id threadID{};
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

    void WriteProfile(const ProfileResult& result) {
        if(!m_OutputStream.is_open()) {
            return;
        }
        if(m_ProfileCount++ > 0)
            m_OutputStream << ",";

        std::string name = result.Name;
        std::replace(std::begin(name), std::end(name), '"', '\'');
        TrimName(name);
        m_OutputStream << "{";
        m_OutputStream << "\"cat\":\"function\",";
        m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
        m_OutputStream << "\"name\":\"" << name << "\",";
        m_OutputStream << "\"ph\":\"X\",";
#ifdef PLATFORM_WINDOWS
        m_OutputStream << "\"pid\": " << static_cast<unsigned long>(::GetCurrentProcessId());
#else
        m_OutputStream << "\"pid\": " << 0ul;
#endif
        m_OutputStream << "\"tid\":" << result.ThreadID << ",";
        m_OutputStream << "\"ts\":" << result.Start;
        m_OutputStream << "}";
    }

    void TrimName(std::string& name) {
        const std::string str_cdecl_conv{"__cdecl"};
        const std::string str_syscall_conv{"__syscall"};
        const std::string str_pascal_conv{"__pascal"};
        const std::string str_stdcall_conv{"__stdcall"};
        const std::string str_fastcall_conv{"__fastcall"};
        const std::string str_vectorcall_conv{"__vectorcall"};
        const std::string str_safecall_conv{"__safecall"};
        const std::string str_clrcall_conv{"__clrcall"};
        const std::string str_void{"void"};
        if(name.starts_with(str_void)) {
            if(const auto iter = name.find(str_void); iter != std::string::npos) {
                name.replace(iter, str_void.size(), " ");
            }
        }
        if(const auto iter = name.find(str_cdecl_conv); iter != std::string::npos) {
            name.replace(iter, str_cdecl_conv.size(), " ");
        }
        if(const auto iter = name.find(str_syscall_conv); iter != std::string::npos) {
            name.replace(iter, str_syscall_conv.size(), " ");
        }
        if(const auto iter = name.find(str_pascal_conv); iter != std::string::npos) {
            name.replace(iter, str_pascal_conv.size(), " ");
        }
        if(const auto iter = name.find(str_stdcall_conv); iter != std::string::npos) {
            name.replace(iter, str_stdcall_conv.size(), " ");
        }
        if(const auto iter = name.find(str_fastcall_conv); iter != std::string::npos) {
            name.replace(iter, str_fastcall_conv.size(), " ");
        }
        if(const auto iter = name.find(str_vectorcall_conv); iter != std::string::npos) {
            name.replace(iter, str_vectorcall_conv.size(), " ");
        }
        if(const auto iter = name.find(str_safecall_conv); iter != std::string::npos) {
            name.replace(iter, str_safecall_conv.size(), " ");
        }
        if(const auto iter = name.find(str_clrcall_conv); iter != std::string::npos) {
            name.replace(iter, str_clrcall_conv.size(), " ");
        }
        name = StringUtils::TrimWhitespace(name);
    }

    void WriteProfileMetaData(const ProfileMetadata& meta, const MetaDataCategory& cat) {
        if(!m_OutputStream.is_open()) {
            return;
        }
        if(m_ProfileCount++ > 0)
            m_OutputStream << ",";

        const auto get_category_name = [&]() {
            switch(cat) {
            case MetaDataCategory::ProcessName:
                return std::string{"process_name"};
            case MetaDataCategory::ProcessLabels:
                return std::string{"process_labels"};
            case MetaDataCategory::ProcessSortIndex:
                return std::string{"process_sort_index"};
            case MetaDataCategory::ThreadName:
                return std::string{"thread_name"};
            case MetaDataCategory::ThreadSortIndex:
                return std::string{"thread_sort_index"};
            default:
                return std::string{"none"};
            }
        };

        const auto get_category_args = [&]() {
            switch(cat) {
            case MetaDataCategory::ProcessName:
                return std::format("\"name\": \"{}\"", meta.processName);
            case MetaDataCategory::ProcessLabels:
                return std::format("\"labels\": \"{}\"", meta.processLabels);
            case MetaDataCategory::ProcessSortIndex:
                return std::format("\"sort_index\": \"{}\"", meta.processSortIndex);
            case MetaDataCategory::ThreadName:
                return std::format("\"name\": \"{}\"", meta.threadName);
            case MetaDataCategory::ThreadSortIndex:
                return std::format("\"sort_index\": \"{}\"", meta.threadSortIndex);
            default:
                return std::string{};
            }
        };

        m_OutputStream << "{";
        m_OutputStream << std::format("\"name\": \"{}\",", get_category_name());
        m_OutputStream << "\"cat\": \"__metadata\",";
        m_OutputStream << "\"ph\": \"M\",";
        #ifdef PLATFORM_WINDOWS
        m_OutputStream << "\"pid\": " << static_cast<unsigned long>(::GetCurrentProcessId()) << ",";
        #else
        m_OutputStream << "\"pid\": " << 0ul << ",";
        #endif
        m_OutputStream << "\"tid\": " << meta.threadID << ',';
        m_OutputStream << "\"args\": {";
        m_OutputStream << get_category_args();
        m_OutputStream << "}";
        m_OutputStream << "}";
    }

    void WriteHeader() {
        m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
    }

    void WriteFooter() {
        m_OutputStream << "]}";
    }

    static Instrumentor& Get() {
        static Instrumentor instance;
        return instance;
    }

private:
    std::unique_ptr<InstrumentationSession> m_CurrentSession{nullptr};
    std::ofstream m_OutputStream{};
    int m_ProfileCount{0};
};

class InstrumentationTimer {
public:
    InstrumentationTimer(const char* name)
    : m_Name(name)
    , m_StartTimepoint(std::chrono::steady_clock::now())
    , m_Stopped(false)
    {
        /* DO NOTHING */
    }

    ~InstrumentationTimer() {
        if(!m_Stopped) {
            Stop();
        }
    }

    void Stop() {
        auto endTimepoint = std::chrono::steady_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        Instrumentor::Get().WriteProfile({m_Name, start, end, std::this_thread::get_id()});

        m_Stopped = true;
    }

private:
    const char* m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
};

#ifdef PROFILE_BUILD
#define PROFILE_BENCHMARK_ADD_METADATA(category, value, thread_id) ProfileBenchmarkMetaData(static_cast<MetaDataCategory>(category), value, thread_id)
#define PROFILE_BENCHMARK_ADD_METADATA_THIS_THREAD(category, value) ProfileBenchmarkMetaData(static_cast<MetaDataCategory>(category), value)
#define PROFILE_BENCHMARK_BEGIN(name, filename) Instrumentor::Get().BeginSession(name, filename)
#define PROFILE_BENCHMARK_END() Instrumentor::Get().EndSession()
#define PROFILE_BENCHMARK_SCOPE(name) InstrumentationTimer TOKEN_PASTE(timer,__LINE__)(name)
#define PROFILE_BENCHMARK_FUNCTION() PROFILE_BENCHMARK_SCOPE(__FUNCSIG__)

namespace detail {

void ProfileBenchmarkMetaData_helper(const MetaDataCategory& category, const std::string& value);

template<typename T, typename std::enable_if_t<std::is_integral_v<T>, bool>>
void ProfileBenchmarkMetaData_helper(const MetaDataCategory& category, const T& value) {
    ProfileMetadata meta{};
    meta.threadID = std::this_thread::get_id();
    switch(category) {
    case MetaDataCategory::ProcessSortIndex:
        meta.processSortIndex = value;
        break;
    case MetaDataCategory::ThreadSortIndex:
        meta.threadSortIndex = value;
        break;
    default:
    /* DO NOTHING */;
    }
    Instrumentor::Get().WriteProfileMetaData(meta, category);
}

} // namespace detail

template<typename T>
void ProfileBenchmarkMetaData(const MetaDataCategory& category, T value, std::thread::id thread_id = std::thread::id{}) {
    ProfileMetadata meta{};
    if(thread_id == std::thread::id{}) {
        meta.threadID = std::this_thread::get_id();
    } else {
        meta.threadID = thread_id;
    }
    if constexpr(std::is_same_v<std::string, T>) {
        detail::ProfileBenchmarkMetaData_helper(category, value);
    } else if constexpr(std::is_same_v<const char*, T>) {
        detail::ProfileBenchmarkMetaData_helper(category, std::string{value ? value : ""});
    } else if constexpr(std::is_integral_v<T>) {
        detail::ProfileBenchmarkMetaData_helper<T>(category, value);
    }
}

//#define PROFILE_BENCHMARK_SET_PROCESS_NAME(value) ProfileBenchmarkMetaData<std::string>(MetaDataCategory::ProcessName, value)
//    #define PROFILE_BENCHMARK_SET_PROCESS_LABELS(value) ProfileBenchmarkMetaData<std::string>(MetaDataCategory::ProcessLabels, value)
//    #define PROFILE_BENCHMARK_SET_PROCESS_SORT_INDEX(value) ProfileBenchmarkMetaData<long long>(MetaDataCategory::ProcessSortIndex, static_cast<long long>(value))
//    #define PROFILE_BENCHMARK_SET_THREAD_NAME(value) ProfileBenchmarkMetaData<std::string>(MetaDataCategory::ThreadName, value)
//    #define PROFILE_BENCHMARK_SET_THREAD_SORT_INDEX(value) ProfileBenchmarkMetaData<long long>(MetaDataCategory::ThreadSortIndex, static_cast<long long>(value))
#else
#define PROFILE_BENCHMARK_ADD_METADATA(category, value, thread_id)
#define PROFILE_BENCHMARK_ADD_METADATA_THIS_THREAD(category, value)
#define PROFILE_BENCHMARK_BEGIN(name, filename)
#define PROFILE_BENCHMARK_END()
#define PROFILE_BENCHMARK_SCOPE(name)
#define PROFILE_BENCHMARK_FUNCTION()
#endif
