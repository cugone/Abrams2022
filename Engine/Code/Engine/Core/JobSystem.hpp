#pragma once

#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/ThreadSafeQueue.hpp"

#include "Engine/Services/IJobSystemService.hpp"

#include "Engine/Core/JobTypes.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class JobSystem : public IJobSystemService {
public:
    JobSystem(int genericCount, std::size_t categoryCount, std::condition_variable* mainJobSignal) noexcept;
    virtual ~JobSystem() noexcept;

    void BeginFrame() noexcept;
    void Shutdown() noexcept;

    void SetCategorySignal(const JobType& category_id, std::condition_variable* signal) noexcept;
    [[nodiscard]] Job* Create(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept;
    void Run(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept;
    void Dispatch(Job* job) noexcept;
    bool Release(Job* job) noexcept;
    void Wait(Job* job) noexcept;
    void DispatchAndRelease(Job* job) noexcept;
    void WaitAndRelease(Job* job) noexcept;
    [[nodiscard]] bool IsRunning() const noexcept;
    [[nodiscard]] std::condition_variable* GetMainJobSignal() const noexcept;

protected:
private:
    void Initialize(int genericCount, std::size_t categoryCount) noexcept;
    void SetIsRunning(bool value = true) noexcept;
    void MainStep() noexcept;
    void GenericJobWorker(std::condition_variable* signal) noexcept;

    static std::vector<ThreadSafeQueue<Job*>*> _queues;
    static std::vector<std::condition_variable*> _signals;
    static std::vector<std::thread> _threads;
    std::condition_variable* _main_job_signal = nullptr;
    std::mutex _cs{};
    std::atomic_bool _is_running = false;
    friend class JobConsumer;
};