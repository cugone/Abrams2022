#include "Engine/Profiling/Instrumentor.hpp"

namespace detail {

void ProfileBenchmarkMetaData_helper(const MetaDataCategory& category, const std::string& value) {
    ProfileMetadata meta{};
    meta.threadID = std::this_thread::get_id();
    switch(category) {
    case MetaDataCategory::ProcessName:
        meta.processName = value;
        break;
    case MetaDataCategory::ProcessLabels:
        meta.processLabels = value;
        break;
    case MetaDataCategory::ThreadName:
        meta.threadName = value;
        break;
    default:
    /* DO NOTHING */;
    }
    Instrumentor::Get().WriteProfileMetaData(meta, category);
}

} // namespace detail
