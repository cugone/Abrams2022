#include "Engine/Core/JobSystem.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Core/ThreadUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Platform/Win.hpp"

#ifdef PROFILE_BUILD
#include <Thirdparty/Tracy/tracy/Tracy.hpp>
#endif

#include <chrono>
#include <string>
#include <sstream>

void JobSystem::GenericJobWorker(std::condition_variable* signal) noexcept {
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
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    Initialize(genericCount, categoryCount);
}

JobSystem::~JobSystem() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    Shutdown();
}

void JobSystem::Initialize(int genericCount, std::size_t categoryCount) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

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
        m_threads[i] = std::move(t);
    }
}

void JobSystem::BeginFrame() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif
    MainStep();
}

void JobSystem::Shutdown() noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

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
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    JobConsumer jc;
    jc.AddCategory(JobType::Main);
    SetCategorySignal(JobType::Main, m_main_job_signal);
    jc.ConsumeAll();
}

void JobSystem::SetCategorySignal(const JobType& category_id, std::condition_variable* signal) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    m_signals[static_cast<std::underlying_type_t<JobType>>(category_id)] = signal;
}

Job* JobSystem::Create(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    auto* j = new Job();
    j->type = category;
    j->state = JobState::Created;
    j->work_cb = cb;
    j->user_data = user_data;
    j->num_dependencies = 1;
    return j;
}

void JobSystem::Run(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    Job* job = Create(category, cb, user_data);
    job->state = JobState::Running;
    DispatchAndRelease(job);
}

void JobSystem::Dispatch(Job* job) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    job->state = JobState::Dispatched;
    ++job->num_dependencies;
    const auto jobtype = TypeUtils::GetUnderlyingValue<JobType>(job->type);
    m_queues[jobtype]->push(job);
    if(auto* signal = m_signals[jobtype]; signal) {
        signal->notify_all();
    }
}

bool JobSystem::Release(Job* job) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    const auto dcount = --job->num_dependencies;
    if(dcount != 0) {
        return false;
    }
    delete job;
    return true;
}

void JobSystem::Wait(Job* job) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    while(job->state != JobState::Finished) {
        std::this_thread::yield();
    }
}

void JobSystem::DispatchAndRelease(Job* job) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    Dispatch(job);
    Release(job);
}

void JobSystem::WaitAndRelease(Job* job) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    Wait(job);
    Release(job);
}

bool JobSystem::IsRunning() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    bool running = m_is_running;
    return running;
}

void JobSystem::SetIsRunning(bool value /*= true*/) noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    m_is_running = value;
}

std::condition_variable* JobSystem::GetMainJobSignal() const noexcept {
#ifdef PROFILE_BUILD
    ZoneScopedC(0xFF0000);
#endif

    return m_main_job_signal;
}
