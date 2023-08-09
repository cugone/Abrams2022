#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Platform/Win.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace TimeUtils {

enum class FormatType {
    Date,
    Time,
    Both
};

std::string_view GetFormatStringFromOptions(const DateTimeStampOptions& options, FormatType format_type);
void AppendStamp(std::ostringstream& msg, const DateTimeStampOptions& options, FormatType format_type);

std::string GetDateTimeStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    std::ostringstream msg;
    AppendStamp(msg, options, TimeUtils::FormatType::Both);
    return msg.str();
}

std::chrono::nanoseconds GetCurrentTimeElapsed() noexcept {
    static auto initial_now = Now<std::chrono::steady_clock>();
    auto now = Now<std::chrono::steady_clock>();
    return (now - initial_now);
}

std::string GetTimeStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    std::ostringstream msg;
    AppendStamp(msg, options, TimeUtils::FormatType::Time);
    return msg.str();
}

std::string GetDateStampFromNow(const DateTimeStampOptions& options /*= DateTimeStampOptions{}*/) noexcept {
    std::ostringstream msg;
    AppendStamp(msg, options, TimeUtils::FormatType::Date);
    return msg.str();
}

void AppendStamp(std::ostringstream& msg, const DateTimeStampOptions& options, FormatType format_type) {
    const auto now = TimeUtils::Now<std::chrono::system_clock>();
    std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};
    std::chrono::zoned_time zt{std::chrono::current_zone(), (now)};
    const auto lt = zt.get_local_time();
    const auto tod = lt - std::chrono::floor<std::chrono::days>(lt);
    const std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(tod)};
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % std::chrono::seconds{1};
    const auto fmt = GetFormatStringFromOptions(options, format_type);
    switch(format_type) {
    case FormatType::Both:
        msg << std::vformat(fmt, std::make_format_args(ymd, hms));
        if(options.include_milliseconds) {
            if(options.use_separator) {
                msg << (options.is_filename ? '_' : ':');
            }
            msg << std::format("{:0>3}", ms.count());
        }
        break;
    case FormatType::Date:
        msg << std::vformat(fmt, std::make_format_args(ymd));
        break;
    case FormatType::Time:
        msg << std::vformat(fmt, std::make_format_args(hms));
        if(options.include_milliseconds) {
            if(options.use_separator) {
                msg << (options.is_filename ? '_' : ':');
            }
            msg << std::format("{:0>3}", ms.count());
        }
        break;
    default:
    /* DO NOTHING */;
    }
}

std::string_view GetFormatStringFromOptions(const DateTimeStampOptions& options, FormatType format_type) {
    switch(format_type) {
    case FormatType::Date:
        return options.use_separator ? "{0:%Y-%m-%d}" : "{0:%Y%m%d}";
    case FormatType::Time:
        return options.use_24_hour_clock ? (options.use_separator ? (options.is_filename ? "{0:%H-%M-%S}" : "{0:%H:%M:%S}") : "{0:%H%M%S}") : (options.use_separator ? (options.is_filename ? "{0:%I-%M-%S}" : "{0:%I:%M:%S}") : "{0:%I%M%S}");
    case FormatType::Both:
        return options.use_24_hour_clock ? (options.use_separator ? (options.is_filename ? "{0:%Y-%m-%d}_{1:%H%M%S}" : "{0:%Y-%m-%d} {1:%H:%M:%S}") : "{0:%Y%m%d}{1:%H%M%S}") : (options.use_separator ? (options.is_filename ? "{0:%Y-%m-%d}_{1:%I%M%S}" : "{0:%Y-%m-%d} {1:%I:%M:%S}") : "{0:%Y%m%d}{1:%I%M%S}");
    default:
        return {};
    }
}

} // namespace TimeUtils
