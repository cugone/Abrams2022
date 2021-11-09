#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Platform/Win.hpp"

#include <ctime>
#include <iomanip>
#include <string_view>
#include <sstream>

namespace TimeUtils {

enum class FormatType {
    Date,
    Time,
    Both
};

std::string_view GetFormatStringFromOptions(const DateTimeStampOptions& options, FormatType type);
void AppendMilliseconds(std::ostringstream& msg, const DateTimeStampOptions& options, const std::chrono::time_point<std::chrono::system_clock> now);
void AppendStamp(std::ostringstream& msg, const DateTimeStampOptions& options, const std::chrono::time_point<std::chrono::system_clock> now);

std::string GetDateTimeStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    using namespace std::chrono;
    auto now = Now<system_clock>();
    std::ostringstream msg;
    AppendStamp(msg, options, now);
    AppendMilliseconds(msg, options, now);
    return msg.str();
}

std::chrono::nanoseconds GetCurrentTimeElapsed() noexcept {
    static auto initial_now = Now<std::chrono::steady_clock>();
    auto now = Now<std::chrono::steady_clock>();
    return (now - initial_now);
}

std::string GetTimeStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    using namespace std::chrono;
    auto now = Now<system_clock>();
    std::ostringstream msg;
    AppendStamp(msg, options, now);
    AppendMilliseconds(msg, options, now);
    return msg.str();
}

std::string GetDateStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    using namespace std::chrono;
    auto now = Now<system_clock>();
    std::ostringstream msg;
    AppendStamp(msg, options, now);
    return msg.str();
}

void AppendStamp(std::ostringstream& msg, const DateTimeStampOptions& options, const std::chrono::time_point<std::chrono::system_clock> now) {
    using namespace std::chrono;
    auto t = system_clock::to_time_t(now);
    std::tm tm;
    ::localtime_s(&tm, &t);
    const auto fmt = GetFormatStringFromOptions(options, FormatType::Both);
    msg << std::put_time(&tm, fmt.data());
}

std::string_view GetFormatStringFromOptions(const DateTimeStampOptions& options, FormatType type) {
    switch(type) {
    case FormatType::Date:
        return options.use_separator ? "%Y-%m-%d" : "%Y%m%d";
    case FormatType::Time:
        return options.use_24_hour_clock ? (options.use_separator ? (options.is_filename ? "%H-%M-%S" : "%H:%M:%S") : "%H%M%S") : (options.use_separator ? (options.is_filename ? "%I-%M-%S" : "%I:%M:%S") : "%I%M%S");
    case FormatType::Both:
        return options.use_24_hour_clock ? (options.use_separator ? (options.is_filename ? "%Y-%m-%d_%H%M%S" : "%Y-%m-%d %H:%M:%S") : "%Y%m%d%H%M%S") : (options.use_separator ? (options.is_filename ? "%Y-%m-%d_%I%M%S" : "%Y-%m-%d %I:%M:%S") : "%Y%m%d%I%M%S");
    default:
        return {};
    }
}

void AppendMilliseconds(std::ostringstream& msg, const DateTimeStampOptions& options, const std::chrono::time_point<std::chrono::system_clock> now) {
    using namespace std::chrono;
    if(options.include_milliseconds) {
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1s;
        if(options.use_separator) {
            msg << (options.is_filename ? "_" : ".");
        }
        msg << std::fixed << std::right << std::setw(3) << std::setfill('0') << ms.count();
    }
}

} // namespace TimeUtils
