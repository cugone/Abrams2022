#pragma once

#include "Engine/Core/ThreadSafeQueue.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <atomic>
#include <functional>
#include <vector>

class Job;
class JobSystem;

enum class JobType : std::size_t {
    Generic,
    Logging,
    Io,
    Render,
    Main,
    Max,
};

enum class JobState : unsigned int {
    None,
    Created,
    Dispatched,
    Enqueued,
    Running,
    Finished,
    Max,
};

class Job {
public:
    Job() noexcept = default;
    ~Job() noexcept;
    JobType type{};
    JobState state{};
    std::function<void(void*)> work_cb;
    void* user_data{};

    void DependencyOf(Job* dependency) noexcept;
    void DependentOn(Job* parent) noexcept;
    void OnDependancyFinished() noexcept;
    void OnFinish() noexcept;

    std::vector<Job*> dependents{};
    std::atomic<unsigned int> num_dependencies{0u};

private:
    void AddDependent(Job* dependent) noexcept;
};

class JobConsumer {
public:
    void AddCategory(const JobType& category) noexcept;
    bool ConsumeJob() noexcept;
    unsigned int ConsumeAll() noexcept;
    void ConsumeFor(TimeUtils::FPMilliseconds consume_duration) noexcept;
    [[nodiscard]] bool HasJobs() const noexcept;

private:
    std::vector<ThreadSafeQueue<Job*>*> _consumables{};
    friend class JobSystem;
};
