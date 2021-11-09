#include "Engine/Game/GameSettings.hpp"

#include "Engine/Core/Config.hpp"

IntVector2 GameSettings::GetWindowResolution() const noexcept {
    return IntVector2{GetWindowWidth(), GetWindowHeight()};
}

int GameSettings::GetWindowWidth() const noexcept {
    return _windowWidth;
}

int GameSettings::GetWindowHeight() const noexcept {
    return _windowHeight;
}

float GameSettings::GetVerticalFov() const noexcept {
    return _fov;
}

void GameSettings::SetWindowResolution(const IntVector2& newResolution) noexcept {
    SetWindowWidth(newResolution.x);
    SetWindowHeight(newResolution.y);
}

void GameSettings::SetWindowWidth(int newWidth) noexcept {
    _windowWidth = newWidth;
}

void GameSettings::SetWindowHeight(int newHeight) noexcept {
    _windowHeight = newHeight;
}

void GameSettings::SetVerticalFov(float newFov) noexcept {
    _fov = newFov;
}

void GameSettings::SetVsyncEnabled(bool newVsync) noexcept {
    _vsync = newVsync;
}

bool GameSettings::IsMouseInvertedX() const noexcept {
    return _invertMouseX;
}

bool GameSettings::IsMouseInvertedY() const noexcept {
    return _invertMouseY;
}

bool GameSettings::IsVsyncEnabled() const noexcept {
    return _vsync;
}

int GameSettings::DefaultWindowWidth() const noexcept {
    return _defaultWindowWidth;
}

int GameSettings::DefaultWindowHeight() const noexcept {
    return _defaultWindowHeight;
}

float GameSettings::DefaultVerticalFov() const noexcept {
    return _defaultFov;
}

bool GameSettings::DefaultMouseInvertedX() const noexcept {
    return _defaultInvertMouseX;
}

bool GameSettings::DefaultMouseInvertedY() const noexcept {
    return _defaultInvertMouseY;
}

bool GameSettings::DefaultVsyncEnabled() const noexcept {
    return _defaultvsync;
}

void GameSettings::SaveToConfig(Config& config) noexcept {
    config.SetValue("width", _windowWidth);
    config.SetValue("height", _windowHeight);
    config.SetValue("vfov", _fov);
    config.SetValue("invertY", _invertMouseY);
    config.SetValue("invertY", _invertMouseX);
    config.SetValue("vsync", _vsync);
}

void GameSettings::SetToDefault() noexcept {
    _windowWidth = _defaultWindowWidth;
    _windowHeight = _defaultWindowHeight;
    _fov = _defaultFov;
    _invertMouseY = _defaultInvertMouseY;
    _invertMouseX = _defaultInvertMouseX;
    _vsync = _defaultvsync;
}
