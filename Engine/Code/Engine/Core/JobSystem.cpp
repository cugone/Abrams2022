#include "Engine/Core/JobSystem.hpp"

#include "Engine/Core/ThreadUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/TypeUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include <chrono>
#include <sstream>

std::vector<ThreadSafeQueue<Job*>*> JobSystem::_queues = std::vector<ThreadSafeQueue<Job*>*>{};
std::vector<std::condition_variable*> JobSystem::_signals = std::vector<std::condition_variable*>{};
std::vector<std::thread> JobSystem::_threads = std::vector<std::thread>{};

void JobSystem::GenericJobWorker(std::condition_variable* signal) noexcept {
    JobConsumer jc;
    jc.AddCategory(JobType::Generic);
    SetCategorySignal(JobType::Generic, signal);
    while(IsRunning()) {
        if(signal) {
            std::unique_lock<std::mutex> lock(_cs);
            //Condition to wake up: Not running or has jobs available
            signal->wait(lock, [&jc, this]() -> bool { return !_is_running || jc.HasJobs(); });
            if(jc.HasJobs()) {
                jc.ConsumeAll();
            }
        }
    }
}

JobSystem::JobSystem(int genericCount, std::size_t categoryCount, std::condition_variable* mainJobSignal) noexcept
: _main_job_signal(mainJobSignal) {
    Initialize(genericCount, categoryCount);
}

JobSystem::~JobSystem() noexcept {
    Shutdown();
}

void JobSystem::Initialize(int genericCount, std::size_t categoryCount) noexcept {
    auto core_count = static_cast<int>(std::thread::hardware_concurrency());
    if(genericCount <= 0) {
        core_count += genericCount;
    }
    --core_count;
    _queues.resize(categoryCount);
    _signals.resize(categoryCount);
    _threads.resize(core_count);
    _is_running = true;

    for(std::size_t i = 0; i < categoryCount; ++i) {
        _queues[i] = new ThreadSafeQueue<Job*>{};
    }

    for(std::size_t i = 0; i < categoryCount; ++i) {
        _signals[i] = nullptr;
    }
    _signals[TypeUtils::GetUnderlyingValue<JobType>(JobType::Generic)] = new std::condition_variable;

    for(std::size_t i = 0; i < static_cast<std::size_t>(core_count); ++i) {
        auto t = std::thread(&JobSystem::GenericJobWorker, this, _signals[TypeUtils::GetUnderlyingValue<JobType>(JobType::Generic)]);
        std::wstring desc{L"Generic Job Thread "};
        desc += std::to_wstring(i);
        ThreadUtils::SetThreadDescription(t, desc);
        _threads[i] = std::move(t);
    }
}

void JobSystem::BeginFrame() noexcept {
    MainStep();
}

void JobSystem::Shutdown() noexcept {
    if(!IsRunning()) {
        return;
    }
    _is_running = false;
    for(auto& signal : _signals) {
        if(signal) {
            signal->notify_all();
        }
    }

    for(auto& thread : _threads) {
        if(thread.joinable()) {
            thread.join();
        }
    }

    for(auto& queue : _queues) {
        delete queue;
        queue = nullptr;
    }
    for(auto& signal : _signals) {
        delete signal;
        signal = nullptr;
    }
    _main_job_signal = nullptr;

    _queues.clear();
    _queues.shrink_to_fit();

    _signals.clear();
    _signals.shrink_to_fit();

    _threads.clear();
    _threads.shrink_to_fit();
}

void JobSystem::MainStep() noexcept {
    JobConsumer jc;
    jc.AddCategory(JobType::Main);
    SetCategorySignal(JobType::Main, _main_job_signal);
    jc.ConsumeAll();
}

void JobSystem::SetCategorySignal(const JobType& category_id, std::condition_variable* signal) noexcept {
    _signals[static_cast<std::underlying_type_t<JobType>>(category_id)] = signal;
}

Job* JobSystem::Create(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
    auto* j = new Job();
    j->type = category;
    j->state = JobState::Created;
    j->work_cb = cb;
    j->user_data = user_data;
    j->num_dependencies = 1;
    return j;
}

void JobSystem::Run(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept {
    Job* job = Create(category, cb, user_data);
    job->state = JobState::Running;
    DispatchAndRelease(job);
}

void JobSystem::Dispatch(Job* job) noexcept {
    job->state = JobState::Dispatched;
    ++job->num_dependencies;
    const auto jobtype = TypeUtils::GetUnderlyingValue<JobType>(job->type);
    _queues[jobtype]->push(job);
    auto* signal = _signals[jobtype];
    if(signal) {
        signal->notify_all();
    }
}

bool JobSystem::Release(Job* job) noexcept {
    const auto dcount = --job->num_dependencies;
    if(dcount != 0) {
        return false;
    }
    delete job;
    return true;
}

void JobSystem::Wait(Job* job) noexcept {
    while(job->state != JobState::Finished) {
        std::this_thread::yield();
    }
}

void JobSystem::DispatchAndRelease(Job* job) noexcept {
    Dispatch(job);
    Release(job);
}

void JobSystem::WaitAndRelease(Job* job) noexcept {
    Wait(job);
    Release(job);
}

bool JobSystem::IsRunning() const noexcept {
    bool running = _is_running;
    return running;
}

void JobSystem::SetIsRunning(bool value /*= true*/) noexcept {
    _is_running = value;
}

std::condition_variable* JobSystem::GetMainJobSignal() const noexcept {
    return _main_job_signal;
}
