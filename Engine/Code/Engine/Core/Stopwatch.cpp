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
    m_interval_time = seconds;
    m_target_time = TimeUtils::FPSeconds{TimeUtils::GetCurrentTimeElapsed()
                                       + seconds};
}

void Stopwatch::SetFrequency(unsigned int hz) noexcept {
    SetSeconds(TimeUtils::FPSeconds(1.0f / static_cast<float>(hz)));
}

bool Stopwatch::Check() const noexcept {
    const auto current_time = TimeUtils::GetCurrentTimeElapsed();
    return (m_target_time < current_time);
}

bool Stopwatch::CheckAndDecrement() noexcept {
    if(Check()) {
        m_target_time += m_interval_time;
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
    m_target_time = TimeUtils::FPSeconds{TimeUtils::GetCurrentTimeElapsed()
                                       + m_interval_time};
}
