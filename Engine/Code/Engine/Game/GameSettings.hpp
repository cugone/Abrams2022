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

    void SetWindowResolution(const IntVector2& newResolution) noexcept;
    void SetWindowWidth(int newWidth) noexcept;
    void SetWindowHeight(int newHeight) noexcept;

    void SetVerticalFov(float newFov) noexcept;

    void SetVsyncEnabled(bool newVsync) noexcept;

    [[nodiscard]] bool IsMouseInvertedX() const noexcept;
    [[nodiscard]] bool IsMouseInvertedY() const noexcept;
    [[nodiscard]] bool IsVsyncEnabled() const noexcept;

    [[nodiscard]] int DefaultWindowWidth() const noexcept;
    [[nodiscard]] int DefaultWindowHeight() const noexcept;

    [[nodiscard]] float DefaultVerticalFov() const noexcept;

    [[nodiscard]] bool DefaultMouseInvertedX() const noexcept;
    [[nodiscard]] bool DefaultMouseInvertedY() const noexcept;
    [[nodiscard]] bool DefaultVsyncEnabled() const noexcept;

    virtual void SaveToConfig(Config& config) noexcept;
    virtual void SetToDefault() noexcept;

protected:
    int _windowWidth = 1600;
    int _defaultWindowWidth = 1600;
    int _windowHeight = 900;
    int _defaultWindowHeight = 900;
    float _fov = 70.0f;
    float _defaultFov = 70.0f;
    bool _invertMouseY = false;
    bool _defaultInvertMouseY = false;
    bool _invertMouseX = false;
    bool _defaultInvertMouseX = false;
    bool _vsync = false;
    bool _defaultvsync = false;
private:
};

template<typename T>
T* GetGameSettingsAs() {
    static GameSettings* currentGameSettings{nullptr};
    return dynamic_cast<T*>(currentGameSettings);
}
