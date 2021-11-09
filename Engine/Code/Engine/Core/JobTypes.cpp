#include "Engine/Core/JobTypes.hpp"

#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/TypeUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IJobSystemService.hpp"


void JobConsumer::AddCategory(const JobType& category) noexcept {
    const auto categoryAsSizeT = TypeUtils::GetUnderlyingValue<JobType>(category);
    if(categoryAsSizeT >= JobSystem::_queues.size()) {
        return;
    }
    if(auto* q = JobSystem::_queues[categoryAsSizeT]; q) {
        _consumables.push_back(q);
    }
}

bool JobConsumer::ConsumeJob() noexcept {
    if(_consumables.empty()) {
        return false;
    }
    for(const auto& consumable : _consumables) {
        if(!consumable) {
            continue;
        }
        auto& queue = *consumable;
        if(queue.empty()) {
            return false;
        }
        auto* job = queue.front();
        queue.pop();
        std::invoke(job->work_cb, job->user_data);
        job->OnFinish();
        job->state = JobState::Finished;
        delete job;
    }
    return true;
}

unsigned int JobConsumer::ConsumeAll() noexcept {
    auto processed_jobs = 0u;
    while(ConsumeJob()) {
        ++processed_jobs;
    }
    return processed_jobs;
}

void JobConsumer::ConsumeFor(TimeUtils::FPMilliseconds consume_duration) noexcept {
    const auto start_time = TimeUtils::Now();
    while(TimeUtils::FPMilliseconds{TimeUtils::Now() - start_time} < consume_duration) {
        ConsumeJob();
    }
}

bool JobConsumer::HasJobs() const noexcept {
    if(_consumables.empty()) {
        return false;
    }
    for(const auto& consumable : _consumables) {
        const auto& queue = *consumable;
        if(!queue.empty()) {
            return true;
        }
    }
    return false;
}

Job::~Job() noexcept {
    delete user_data;
}

void Job::DependencyOf(Job* dependency) noexcept {
    DependentOn(dependency);
}

void Job::DependentOn(Job* parent) noexcept {
    parent->AddDependent(this);
}

void Job::OnDependancyFinished() noexcept {
    ++num_dependencies;
    ServiceLocator::get<IJobSystemService>().DispatchAndRelease(this);
}

void Job::OnFinish() noexcept {
    for(auto& dependent : dependents) {
        dependent->OnDependancyFinished();
    }
}

void Job::AddDependent(Job* dependent) noexcept {
    dependent->state = JobState::Enqueued;
    dependents.push_back(dependent);
}
