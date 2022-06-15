#pragma once

#include "Engine/Services/IService.hpp"

#include <condition_variable>
#include <functional>

enum class JobType : std::size_t;
class Job;

class IJobSystemService : public IService {
public:
    virtual ~IJobSystemService() noexcept {/* DO NOTHING */};

    virtual void BeginFrame() noexcept = 0;
    virtual void Shutdown() noexcept = 0;

    virtual void SetCategorySignal(const JobType& category_id, std::condition_variable* signal) noexcept = 0;
    [[nodiscard]] virtual Job* Create(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept = 0;
    virtual void Run(const JobType& category, const std::function<void(void*)>& cb, void* user_data) noexcept = 0;
    virtual void Dispatch(Job* job) noexcept = 0;
    virtual bool Release(Job* job) noexcept = 0;
    virtual void Wait(Job* job) noexcept = 0;
    virtual void DispatchAndRelease(Job* job) noexcept = 0;
    virtual void WaitAndRelease(Job* job) noexcept = 0;
    [[nodiscard]] virtual bool IsRunning() const noexcept = 0;
    virtual void SetIsRunning(bool value = true) noexcept = 0;

    [[nodiscard]] virtual std::condition_variable* GetMainJobSignal() const noexcept = 0;

protected:
private:
};

class NullJobSystemService : public IJobSystemService {
public:
    virtual ~NullJobSystemService() noexcept {/* DO NOTHING */};

    void BeginFrame() noexcept override {}
    void Shutdown() noexcept override {}

    void SetCategorySignal([[maybe_unused]] const JobType& category_id, [[maybe_unused]] std::condition_variable* signal) noexcept override {}
    [[nodiscard]] Job* Create([[maybe_unused]] const JobType& category, [[maybe_unused]] const std::function<void(void*)>& cb, [[maybe_unused]] void* user_data) noexcept override { return nullptr; }
    void Run([[maybe_unused]] const JobType& category, [[maybe_unused]] const std::function<void(void*)>& cb, [[maybe_unused]] void* user_data) noexcept override {}
    void Dispatch([[maybe_unused]] Job* job) noexcept override {}
    [[nodiscard]] bool Release([[maybe_unused]] Job* job) noexcept override { return false; }
    void Wait([[maybe_unused]] Job* job) noexcept override {}
    void DispatchAndRelease([[maybe_unused]] Job* job) noexcept override {}
    void WaitAndRelease([[maybe_unused]] Job* job) noexcept override {}
    [[nodiscard]] bool IsRunning() const noexcept override { return false; }
    void SetIsRunning([[maybe_unused]] bool value = true) noexcept override {}

    [[nodiscard]] std::condition_variable* GetMainJobSignal() const noexcept override { return nullptr; }

protected:
private:
};
