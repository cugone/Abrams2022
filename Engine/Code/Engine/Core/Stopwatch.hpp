#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include <chrono>
#include <ratio>

class Stopwatch {
public:
    Stopwatch() = default;
    Stopwatch(const Stopwatch& other) = default;
    Stopwatch(Stopwatch&& r_other) = default;
    Stopwatch& operator=(const Stopwatch& rhs) = default;
    Stopwatch& operator=(Stopwatch&& rhs) = default;
    explicit Stopwatch(const TimeUtils::FPSeconds& seconds) noexcept;
    explicit Stopwatch(unsigned int frequency) noexcept;
    explicit Stopwatch(float seconds) noexcept;
    ~Stopwatch() = default;

    void SetSeconds(const TimeUtils::FPSeconds& seconds) noexcept;
    void SetFrequency(unsigned int hz) noexcept;
    [[nodiscard]] bool Check() const noexcept;
    [[nodiscard]] bool CheckAndDecrement() noexcept;
    [[nodiscard]] bool CheckAndReset() noexcept;
    [[nodiscard]] unsigned int DecrementAll() noexcept;
    void Reset() noexcept;

private:
    TimeUtils::FPSeconds interval_time{};
    TimeUtils::FPSeconds target_time{};
};