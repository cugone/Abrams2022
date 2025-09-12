#pragma once

#include "Engine/Math/IntVector2.hpp"

class Config;

class GameSettings {
public:
    GameSettings() noexcept = default;
    GameSettings(const GameSettings& other) noexcept = default;
    GameSettings(GameSettings&& other) noexcept = default;
    virtual ~GameSettings() noexcept = default;

    GameSettings& operator=(const GameSettings& rhs) noexcept = default;
    GameSettings& operator=(GameSettings&& rhs) noexcept = default;

    [[nodiscard]] IntVector2 GetWindowResolution() const noexcept;
    [[nodiscard]] int GetWindowWidth() const noexcept;
    [[nodiscard]] int GetWindowHeight() const noexcept;

    [[nodiscard]] float GetVerticalFov() const noexcept;

    bool IsWindowed() const noexcept;
    bool IsFullscreen() const noexcept;
    void SetWindowedMode() noexcept;
    void SetFullscreenMode() noexcept;

    void SetWindowResolution(const IntVector2& newResolution) noexcept;
    void SetWindowWidth(int newWidth) noexcept;
    void SetWindowHeight(int newHeight) noexcept;

    void SetVerticalFov(float newFov) noexcept;

    void SetVsyncEnabled(bool newVsync) noexcept;

    [[nodiscard]] bool IsMouseInvertedX() const noexcept;
    [[nodiscard]] bool IsMouseInvertedY() const noexcept;
    [[nodiscard]] bool IsVsyncEnabled() const noexcept;

    [[nodiscard]] bool DefaultWindowedMode() const noexcept;

    [[nodiscard]] int DefaultWindowWidth() const noexcept;
    [[nodiscard]] int DefaultWindowHeight() const noexcept;

    [[nodiscard]] float DefaultVerticalFov() const noexcept;

    [[nodiscard]] bool DefaultMouseInvertedX() const noexcept;
    [[nodiscard]] bool DefaultMouseInvertedY() const noexcept;
    [[nodiscard]] bool DefaultVsyncEnabled() const noexcept;

    virtual void SaveToConfig(Config& config) noexcept;
    virtual void SetToDefault() noexcept;

protected:
    int m_windowWidth = 1600;
    int m_defaultWindowWidth = 1600;
    int m_windowHeight = 900;
    int m_defaultWindowHeight = 900;
    float m_fov = 70.0f;
    float m_defaultFov = 70.0f;
    bool m_windowed = true;
    bool m_defaultWindowed = true;
    bool m_invertMouseY = false;
    bool m_defaultInvertMouseY = false;
    bool m_invertMouseX = false;
    bool m_defaultInvertMouseX = false;
    bool m_vsync = false;
    bool m_defaultvsync = false;
private:
};
