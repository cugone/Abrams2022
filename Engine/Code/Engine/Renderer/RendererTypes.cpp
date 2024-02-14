#include "Engine/Renderer/RendererTypes.hpp"

#include <format>

screenshot_job_t::screenshot_job_t()
: m_saveLocation{FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::EngineData) / std::filesystem::path{"Screenshots"}} {
    namespace FS = std::filesystem;
    (void)FileUtils::CreateFolders(m_saveLocation);
    const std::filesystem::path folder = m_saveLocation;
    const auto screenshot_count = std::size_t{1u} + FileUtils::CountFilesInFolders(folder);
    const auto filepath = folder / FS::path{std::format("Screenshot_{}.png", screenshot_count)};
    m_saveLocation = filepath;
}

screenshot_job_t::screenshot_job_t(std::filesystem::path location)
: m_saveLocation{location} {
    /* DO NOTHING */
}

screenshot_job_t::screenshot_job_t(std::string location)
: m_saveLocation{location} {
    /* DO NOTHING */
}

screenshot_job_t::operator bool() const noexcept {
    return !m_saveLocation.empty() && std::filesystem::exists(m_saveLocation.parent_path());
}

screenshot_job_t::operator std::string() const noexcept {
    return m_saveLocation.string();
}

screenshot_job_t::operator std::filesystem::path() const noexcept {
    return m_saveLocation;
}

void screenshot_job_t::clear() noexcept {
    m_saveLocation.clear();
}
