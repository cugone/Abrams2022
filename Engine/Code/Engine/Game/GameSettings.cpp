#include "Engine/Game/GameSettings.hpp"

#include "Engine/Core/Config.hpp"

IntVector2 GameSettings::GetWindowResolution() const noexcept {
    return IntVector2{GetWindowWidth(), GetWindowHeight()};
}

int GameSettings::GetWindowWidth() const noexcept {
    return m_windowWidth;
}

int GameSettings::GetWindowHeight() const noexcept {
    return m_windowHeight;
}

float GameSettings::GetVerticalFov() const noexcept {
    return m_fov;
}

void GameSettings::SetWindowResolution(const IntVector2& newResolution) noexcept {
    SetWindowWidth(newResolution.x);
    SetWindowHeight(newResolution.y);
}

void GameSettings::SetWindowWidth(int newWidth) noexcept {
    m_windowWidth = newWidth;
}

void GameSettings::SetWindowHeight(int newHeight) noexcept {
    m_windowHeight = newHeight;
}

void GameSettings::SetVerticalFov(float newFov) noexcept {
    m_fov = newFov;
}

void GameSettings::SetVsyncEnabled(bool newVsync) noexcept {
    m_vsync = newVsync;
}

bool GameSettings::IsMouseInvertedX() const noexcept {
    return m_invertMouseX;
}

bool GameSettings::IsMouseInvertedY() const noexcept {
    return m_invertMouseY;
}

bool GameSettings::IsVsyncEnabled() const noexcept {
    return m_vsync;
}

int GameSettings::DefaultWindowWidth() const noexcept {
    return m_defaultWindowWidth;
}

int GameSettings::DefaultWindowHeight() const noexcept {
    return m_defaultWindowHeight;
}

float GameSettings::DefaultVerticalFov() const noexcept {
    return m_defaultFov;
}

bool GameSettings::DefaultMouseInvertedX() const noexcept {
    return m_defaultInvertMouseX;
}

bool GameSettings::DefaultMouseInvertedY() const noexcept {
    return m_defaultInvertMouseY;
}

bool GameSettings::DefaultVsyncEnabled() const noexcept {
    return m_defaultvsync;
}

void GameSettings::SaveToConfig(Config& config) noexcept {
    config.SetValue("width", m_windowWidth);
    config.SetValue("height", m_windowHeight);
    config.SetValue("vfov", m_fov);
    config.SetValue("invertY", m_invertMouseY);
    config.SetValue("invertY", m_invertMouseX);
    config.SetValue("vsync", m_vsync);
}

void GameSettings::SetToDefault() noexcept {
    m_windowWidth = m_defaultWindowWidth;
    m_windowHeight = m_defaultWindowHeight;
    m_fov = m_defaultFov;
    m_invertMouseY = m_defaultInvertMouseY;
    m_invertMouseX = m_defaultInvertMouseX;
    m_vsync = m_defaultvsync;
}
