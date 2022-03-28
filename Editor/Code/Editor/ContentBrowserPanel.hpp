#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Editor/IGPanel.hpp"

#include <filesystem>
#include <mutex>
#include <vector>

//class Editor {
//public:
//    enum class AssetType;
//};

class ContentBrowserPanel : public IGPanel {
public:
    virtual ~ContentBrowserPanel() = default;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void UpdateContentBrowserPaths() noexcept;

    std::filesystem::path currentDirectory{};

protected:
private:
    struct ContentBrowserItemStats {
        //Editor::AssetType asset_type{};
        std::string asset_type{};
        std::string id{};
        std::filesystem::path path{};
        std::size_t filesize{0u};
    };

    void ShowContextMenuOnEmptySpace() noexcept;
    void PollContentBrowserPaths() noexcept;
    void ShowHoveredItemStats(const ContentBrowserItemStats& stats) noexcept;

    mutable std::mutex m_cs;
    std::vector<std::filesystem::path> m_PathsCache{};
    Stopwatch m_UpdatePoll{1.0f};
    uint32_t m_PanelWidth{1600u};
    bool m_CacheNeedsImmediateUpdate{true};
};
