#pragma once

#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Game/GameBase.hpp"

#include "Engine/Renderer/FrameBuffer.hpp"

#include "Editor/ContentBrowserPanel.hpp"

#include <filesystem>
#include <vector>

class Texture;

class Editor : public GameBase {
public:

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void HandleWindowResize(unsigned int newWidth, unsigned int newHeight) noexcept override;

    const GameSettings& GetSettings() const noexcept override;
    GameSettings& GetSettings() noexcept override;

    Texture* GetAssetTextureFromType(const std::filesystem::path& path) const noexcept;
protected:
private:

    void ShowUI([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowWorldInspectorWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowSettingsWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowSelectedEntityComponents([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowPropertiesWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowMainViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowContentBrowserWindow([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void HandleInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void HandleMenuKeyboardInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void HandleCameraInput([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    void DoFileNew() noexcept;
    void DoFileOpen() noexcept;
    void DoFileSaveAs() noexcept;
    void DoFileSave() noexcept;

    bool IsSceneLoaded() const noexcept;

    bool HasAssetExtension(const std::filesystem::path& path) const noexcept;
    bool IsAssetExtension(const std::filesystem::path& ext) const noexcept;
    bool IsImageAssetExtension(const std::filesystem::path& ext) const noexcept;

    OrthographicCameraController m_editorCamera{};
    ContentBrowserPanel m_ContentBrowser{};
    uint32_t m_ViewportWidth{1600u};
    uint32_t m_ViewportHeight{900u};
    bool m_IsViewportWindowActive{false};
    std::shared_ptr<FrameBuffer> buffer{};
};
