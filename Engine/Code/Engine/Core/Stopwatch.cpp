#include "Engine/Core/Stopwatch.hpp"

Stopwatch::Stopwatch(unsigned int frequency) noexcept
: Stopwatch(TimeUtils::FPSeconds(1.0f / static_cast<float>(frequency))) {
    /* DO NOTHING */
}

Stopwatch::Stopwatch(const TimeUtils::FPSeconds& seconds) noexcept {
    SetSeconds(seconds);
}

Stopwatch::Stopwatch(float seconds) noexcept
: Stopwatch(TimeUtils::FPSeconds(seconds)) {
    /* DO NOTHING */
}

void Stopwatch::SetSeconds(const TimeUtils::FPSeconds& seconds) noexcept {
    interval_time = seconds;
    target_time = TimeUtils::FPSeconds{TimeUtils::GetCurrentTimeElapsed()
                                       + seconds};
}

void Stopwatch::SetFrequency(unsigned int hz) noexcept {
    SetSeconds(TimeUtils::FPSeconds(1.0f / static_cast<float>(hz)));
}

bool Stopwatch::Check() const noexcept {
    const auto current_time = TimeUtils::GetCurrentTimeElapsed();
    return (target_time < current_time);
}

bool Stopwatch::CheckAndDecrement() noexcept {
    if(Check()) {
        target_time += interval_time;
        return true;
    } else {
        return false;
    }
}

bool Stopwatch::CheckAndReset() noexcept {
    if(Check()) {
        Reset();
        return true;
    } else {
        return false;
    }
}

unsigned int Stopwatch::DecrementAll() noexcept {
    unsigned int count = 0;
    while(CheckAndDecrement()) {
        ++count;
    }

    return count;
}

void Stopwatch::Reset() noexcept {
    target_time = TimeUtils::FPSeconds{TimeUtils::GetCurrentTimeElapsed()
                                       + interval_time};
}
