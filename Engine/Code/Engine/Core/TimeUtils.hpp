#pragma once

#include <chrono>
#include <string>

namespace TimeUtils {

using FPSeconds = std::chrono::duration<float>;
using FPMilliseconds = std::chrono::duration<float, std::milli>;
using FPMicroseconds = std::chrono::duration<float, std::micro>;
using FPNanoseconds = std::chrono::duration<float, std::nano>;
using FPFrames = std::chrono::duration<float, std::ratio<1, 60>>;
using Frames = std::chrono::duration<uint64_t, std::ratio<1, 60>>;

template<typename Clock = std::chrono::steady_clock>
[[nodiscard]] decltype(auto) Now() noexcept {
    return Clock::now();
}

//See https://www.youtube.com/watch?v=HTiNq95S3TM&t=1h15m36s
class ProgramClock {
public:
    using innerClock = typename std::chrono::steady_clock;

    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<ProgramClock>;
    static inline const bool is_steady = innerClock::is_steady;

    static innerClock::time_point programStart;
    static time_point now() noexcept {
        return time_point(std::chrono::duration_cast<duration>(innerClock::now() - programStart));
    }
};

[[nodiscard]] std::chrono::nanoseconds GetCurrentTimeElapsed() noexcept;

struct DateTimeStampOptions {
    bool use_separator = false;
    bool use_24_hour_clock = true;
    bool include_milliseconds = true;
    bool is_filename = true;
};

[[nodiscard]] std::string GetDateTimeStampFromNow(const DateTimeStampOptions& options = DateTimeStampOptions{}) noexcept;
[[nodiscard]] std::string GetTimeStampFromNow(const DateTimeStampOptions& options = DateTimeStampOptions{}) noexcept;
[[nodiscard]] std::string GetDateStampFromNow(const DateTimeStampOptions& options = DateTimeStampOptions{}) noexcept;

} // namespace TimeUtils