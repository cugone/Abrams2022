#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Editor/IGPanel.hpp"

#include <filesystem>
#include <mutex>
#include <vector>

class ContentBrowserPanel : public IGPanel {
public:
    virtual ~ContentBrowserPanel() = default;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void UpdateContentBrowserPaths() noexcept;

    std::filesystem::path currentDirectory{};
protected:
private:
    void ShowContextMenuOnEmptySpace() noexcept;
    void PollContentBrowserPaths() noexcept;

    mutable std::mutex _cs;
    std::vector<std::filesystem::path> m_PathsCache{};
    Stopwatch m_UpdatePoll{1.0f};
    uint32_t m_PanelWidth{1600u};
    bool m_CacheNeedsImmediateUpdate{true};
};
