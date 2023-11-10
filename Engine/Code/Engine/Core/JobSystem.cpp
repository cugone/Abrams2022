#include "Engine/Core/JobSystem.hpp"

#include "Engine/Core/ThreadUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

#include <chrono>
#include <sstream>

void JobSystem::GenericJobWorker(std::condition_variable* signal) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    JobConsumer jc;
    jc.AddCategory(JobType::Generic);
    SetCategorySignal(JobType::Generic, signal);
    while(IsRunning()) {
        if(signal) {
            std::unique_lock<std::mutex> lock(m_cs);
            //Condition to wake up: Not running or has jobs available
            signal->wait(lock, [&jc, this]() -> bool { return !m_is_running || jc.HasJobs(); });
            if(jc.HasJobs()) {
                jc.ConsumeAll();
            }
        }
    }
}

JobSystem::JobSystem(int genericCount, std::size_t categoryCount, std::unique_ptr<std::condition_variable> mainJobSignal) noexcept
: m_main_job_signal(mainJobSignal.release()) {
    PROFILE_BENCHMARK_FUNCTION();
    Initialize(genericCount, categoryCount);
}

JobSystem::~JobSystem() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    Shutdown();
}

void JobSystem::Initialize(int genericCount, std::size_t categoryCount) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    auto core_count = static_cast<int>(std::thread::hardware_concurrency());
    if(genericCount <= 0) {
        core_count += genericCount;
    }
    --core_count;
    m_queues.resize(categoryCount);
    m_signals.resize(categoryCount);
    m_threads.resize(core_count);
    m_is_running = true;

    for(std::size_t i = 0; i < categoryCount; ++i) {
        m_queues[i] = std::make_unique<ThreadSafeQueue<Job*>>();
    }
    for(std::size_t i = 0; i < categoryCount; ++i) {
        m_signals[i] = nullptr;
    }
    m_signals[TypeUtils::GetUnderlyingValue<JobType>(JobType::Generic)] = new std::condition_variable;

    for(std::size_t i = 0; i < static_cast<std::size_t>(core_count); ++i) {
        auto t = std::jthread(&JobSystem::GenericJobWorker, this, m_signals[TypeUtils::GetUnderlyingValue<JobType>(JobType::Generic)]);
        std::string thread_desc = std::format("Generic Job Thread {}", i);
        ThreadUtils::SetThreadDescription(t, thread_desc);
        ProfileMetadata metadata{};
        metadata.threadName = thread_desc;
        metadata.threadID = t.get_id();
        metadata.threadSortIndex = i + 2;
        metadata.ProcessID = ThreadUtils::GetProcessIDFromThread(t);
        Instrumentor::Get().WriteSessionData(MetaDataCategory::ThreadName, metadata);
        Instrumentor::Get().WriteSessionData(MetaDataCategory::ThreadSortIndex, metadata);
        m_threads[i] = std::move(t);
    }
}

void JobSystem::BeginFrame() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    MainStep();
}

void JobSystem::Shutdown() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(!IsRunning()) {
        return;
    }
    m_is_running = false;
    for(auto& signal : m_signals) {
        if(signal) {
            signal->notify_all();
        }
    }

    for(auto& thread : m_threads) {
        if(thread.joinable()) {
            thread.join();
        }
    }

    for(auto& queue : m_queues) {
        queue.reset();
    }
    for(auto& signal : m_signals) {
        delete signal;
        signal = nullptr;
    }
    m_main_job_signal = nullptr;

    m_queues.clear();
    m_queues.shrink_to_fit();

    m_signals.clear();
    m_signals.shrink_to_fit();

    m_threads.clear();
    m_threads.shrink_to_fit();
}

void JobSystem::MainStep() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    JobConsumer jc;
    jc.AddCategory(JobType::Main);
    SetCategorySignal(JobType::Main, m_main_job_signal);
    jc.ConsumeAll();
}

void JobSystem::SetCategorySignal(const JobType& category_id, std::condition_variable* signal) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    m_signals[static_cast<std::underlying_type_t<JobType>>(category_id)] = signal;
}

Job* JobSystem::Create(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    auto* j = new Job();
    j->type = category;
    j->state = JobState::Created;
    j->work_cb = cb;
    j->user_data = user_data;
    j->num_dependencies = 1;
    return j;
}

void JobSystem::Run(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    Job* job = Create(category, cb, user_data);
    job->state = JobState::Running;
    DispatchAndRelease(job);
}

void JobSystem::Dispatch(Job* job) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    job->state = JobState::Dispatched;
    ++job->num_dependencies;
    const auto jobtype = TypeUtils::GetUnderlyingValue<JobType>(job->type);
    m_queues[jobtype]->push(job);
    if(auto* signal = m_signals[jobtype]; signal) {
        signal->notify_all();
    }
}

bool JobSystem::Release(Job* job) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    const auto dcount = --job->num_dependencies;
    if(dcount != 0) {
        return false;
    }
    delete job;
    return true;
}

void JobSystem::Wait(Job* job) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    while(job->state != JobState::Finished) {
        std::this_thread::yield();
    }
}

void JobSystem::DispatchAndRelease(Job* job) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    Dispatch(job);
    Release(job);
}

void JobSystem::WaitAndRelease(Job* job) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    Wait(job);
    Release(job);
}

bool JobSystem::IsRunning() const noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    bool running = m_is_running;
    return running;
}

void JobSystem::SetIsRunning(bool value /*= true*/) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    m_is_running = value;
}

std::condition_variable* JobSystem::GetMainJobSignal() const noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    return m_main_job_signal;
}
