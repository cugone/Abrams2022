#pragma once

#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/ThreadSafeQueue.hpp"

#include "Engine/Services/IJobSystemService.hpp"

#include "Engine/Core/JobTypes.hpp"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class JobSystem : public IJobSystemService {
public:
    JobSystem(int genericCount, std::size_t categoryCount, std::unique_ptr<std::condition_variable> mainJobSignal) noexcept;
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

    static inline std::vector<std::unique_ptr<ThreadSafeQueue<Job*>>> m_queues = std::vector<std::unique_ptr<ThreadSafeQueue<Job*>>>{};
    static inline std::vector<std::condition_variable*> m_signals = std::vector<std::condition_variable*>{};
    static inline std::vector<std::jthread> m_threads = std::vector<std::jthread>{};
    std::condition_variable* m_main_job_signal{};
    std::mutex m_cs{};
    std::atomic_bool m_is_running = false;
    friend class JobConsumer;
};