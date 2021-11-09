#pragma once

#include "Engine/Services/IService.hpp"

#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/Vector2.hpp"

class Window;
class XboxController;

enum class KeyCode;

class IInputService : public IService {
public:
    virtual ~IInputService() noexcept {};

    [[nodiscard]] virtual bool WasAnyKeyPressed() const noexcept = 0;
    [[nodiscard]] virtual bool WasAnyMouseButtonPressed() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseMoved() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseJustUsed() const noexcept = 0;
    [[nodiscard]] virtual bool IsKeyUp([[maybe_unused]] const KeyCode& key) const noexcept = 0;
    [[nodiscard]] virtual bool WasKeyJustPressed([[maybe_unused]] const KeyCode& key) const noexcept = 0;
    [[nodiscard]] virtual bool IsKeyDown([[maybe_unused]] const KeyCode& key) const noexcept = 0;
    [[nodiscard]] virtual bool WasKeyJustPressedOrIsKeyDown([[maybe_unused]] const KeyCode& key) const noexcept = 0;
    [[nodiscard]] virtual bool IsAnyKeyDown() const noexcept = 0;
    [[nodiscard]] virtual bool WasKeyJustReleased([[maybe_unused]] const KeyCode& key) const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseWheelJustScrolledUp() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseWheelJustScrolledDown() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseWheelJustScrolledLeft() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseWheelJustScrolledRight() const noexcept = 0;
    [[nodiscard]] virtual bool WasMouseWheelJustUsed() const noexcept = 0;

    [[nodiscard]] virtual std::size_t GetConnectedControllerCount() const noexcept = 0;
    [[nodiscard]] virtual bool IsAnyControllerConnected() const noexcept = 0;
    [[nodiscard]] virtual const XboxController& GetXboxController([[maybe_unused]] const std::size_t& controllerIndex) const noexcept = 0;
    [[nodiscard]] virtual XboxController& GetXboxController([[maybe_unused]] const std::size_t& controllerIndex) noexcept = 0;

    [[nodiscard]] virtual bool IsMouseCursorVisible() const noexcept = 0;
    virtual void ToggleMouseCursorVisibility() noexcept = 0;
    virtual void HideMouseCursor() noexcept = 0;
    virtual void ShowMouseCursor() noexcept = 0;

    virtual void SetCursorScreenPosition([[maybe_unused]] const Vector2& screen_pos) noexcept = 0;
    [[nodiscard]] virtual Vector2 GetCursorScreenPosition() const noexcept = 0;

    virtual void SetCursorWindowPosition([[maybe_unused]] const Window& window, const Vector2& window_pos) noexcept = 0;
    [[nodiscard]] virtual Vector2 GetCursorWindowPosition([[maybe_unused]] const Window& window_ref) const noexcept = 0;

    virtual void SetCursorWindowPosition([[maybe_unused]] const Vector2& window_pos) noexcept = 0;
    [[nodiscard]] virtual Vector2 GetCursorWindowPosition() noexcept = 0;

    virtual void SetCursorToScreenCenter() noexcept = 0;
    virtual void SetCursorToWindowCenter([[maybe_unused]] const Window& window_ref) noexcept = 0;
    virtual void SetCursorToWindowCenter() noexcept = 0;

    [[nodiscard]] virtual Vector2 GetMouseDeltaFromWindowCenter() const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetMouseDeltaFromWindowCenter([[maybe_unused]] const Window& window_ref) const noexcept = 0;
    [[nodiscard]] virtual const Vector2& GetMouseCoords() const noexcept = 0;
    [[nodiscard]] virtual const Vector2& GetMouseDelta() const noexcept = 0;

    [[nodiscard]] virtual int GetMouseWheelPosition() const noexcept = 0;
    [[nodiscard]] virtual int GetMouseWheelPositionNormalized() const noexcept = 0;

    [[nodiscard]] virtual int GetMouseWheelHorizontalPosition() const noexcept = 0;
    [[nodiscard]] virtual int GetMouseWheelHorizontalPositionNormalized() const noexcept = 0;

    [[nodiscard]] virtual IntVector2 GetMouseWheelPositionAsIntVector2() const noexcept = 0;

    [[nodiscard]] virtual bool IsMouseLockedToViewport() const noexcept = 0;
    virtual void LockMouseToViewport([[maybe_unused]] const Window& window) const noexcept = 0;
    virtual void LockMouseToWindowViewport() const noexcept = 0;
    virtual void UnlockMouseFromViewport() const noexcept = 0;

    [[nodiscard]] virtual Vector2 GetScreenCenter() const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetWindowCenter() const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetWindowCenter([[maybe_unused]] const Window& window) const noexcept = 0;
    [[nodiscard]] virtual bool WasAnyControllerJustUsed() const noexcept = 0;

protected:
private:
    
};

class NullInputService : public IInputService {
public:
    virtual ~NullInputService() noexcept {};

    [[nodiscard]] bool WasAnyKeyPressed() const noexcept override {}
    [[nodiscard]] bool WasAnyMouseButtonPressed() const noexcept override {}
    [[nodiscard]] bool WasMouseMoved() const noexcept override {}
    [[nodiscard]] bool WasMouseJustUsed() const noexcept override {}
    [[nodiscard]] bool IsKeyUp([[maybe_unused]] const KeyCode& key) const noexcept override {}
    [[nodiscard]] bool WasKeyJustPressed([[maybe_unused]] const KeyCode& key) const noexcept override {}
    [[nodiscard]] bool IsKeyDown([[maybe_unused]] const KeyCode& key) const noexcept override {}
    [[nodiscard]] bool WasKeyJustPressedOrIsKeyDown([[maybe_unused]] const KeyCode& key) const noexcept override {}
    [[nodiscard]] bool IsAnyKeyDown() const noexcept override {}
    [[nodiscard]] bool WasKeyJustReleased([[maybe_unused]] const KeyCode& key) const noexcept override {}
    [[nodiscard]] bool WasMouseWheelJustScrolledUp() const noexcept override {}
    [[nodiscard]] bool WasMouseWheelJustScrolledDown() const noexcept override {}
    [[nodiscard]] bool WasMouseWheelJustScrolledLeft() const noexcept override {}
    [[nodiscard]] bool WasMouseWheelJustScrolledRight() const noexcept override {}
    [[nodiscard]] bool WasMouseWheelJustUsed() const noexcept override {}
    [[nodiscard]] std::size_t GetConnectedControllerCount() const noexcept override {}
    [[nodiscard]] bool IsAnyControllerConnected() const noexcept override {}
    [[nodiscard]] const XboxController& GetXboxController([[maybe_unused]] const std::size_t& controllerIndex) const noexcept override {}
    [[nodiscard]] XboxController& GetXboxController([[maybe_unused]] const std::size_t& controllerIndex) noexcept override {}
    [[nodiscard]] bool IsMouseCursorVisible() const noexcept override {}
    void ToggleMouseCursorVisibility() noexcept override {}
    void HideMouseCursor() noexcept override {}
    void ShowMouseCursor() noexcept override {}
    void SetCursorScreenPosition([[maybe_unused]] const Vector2& screen_pos) noexcept override {}
    [[nodiscard]] Vector2 GetCursorScreenPosition() const noexcept override {}
    void SetCursorWindowPosition([[maybe_unused]] const Window& window, [[maybe_unused]] const Vector2& window_pos) noexcept override {}
    [[nodiscard]] Vector2 GetCursorWindowPosition([[maybe_unused]] const Window& window_ref) const noexcept override {}
    void SetCursorWindowPosition([[maybe_unused]] const Vector2& window_pos) noexcept override {}
    [[nodiscard]] Vector2 GetCursorWindowPosition() noexcept override {}
    void SetCursorToScreenCenter() noexcept override {}
    void SetCursorToWindowCenter([[maybe_unused]] const Window& window_ref) noexcept override {}
    void SetCursorToWindowCenter() noexcept override {}
    [[nodiscard]] Vector2 GetMouseDeltaFromWindowCenter() const noexcept override {}
    [[nodiscard]] Vector2 GetMouseDeltaFromWindowCenter([[maybe_unused]] const Window& window_ref) const noexcept override {}
    [[nodiscard]] const Vector2& GetMouseCoords() const noexcept override {}
    [[nodiscard]] const Vector2& GetMouseDelta() const noexcept override {}
    [[nodiscard]] int GetMouseWheelPosition() const noexcept override {}
    [[nodiscard]] int GetMouseWheelPositionNormalized() const noexcept override {}
    [[nodiscard]] int GetMouseWheelHorizontalPosition() const noexcept override {}
    [[nodiscard]] int GetMouseWheelHorizontalPositionNormalized() const noexcept override {}
    [[nodiscard]] IntVector2 GetMouseWheelPositionAsIntVector2() const noexcept override {}
    [[nodiscard]] bool IsMouseLockedToViewport() const noexcept override {}
    void LockMouseToViewport([[maybe_unused]] const Window& window) const noexcept override {}
    void LockMouseToWindowViewport() const noexcept override {}
    void UnlockMouseFromViewport() const noexcept override {}
    [[nodiscard]] Vector2 GetScreenCenter() const noexcept override {}
    [[nodiscard]] Vector2 GetWindowCenter() const noexcept override {}
    [[nodiscard]] Vector2 GetWindowCenter([[maybe_unused]] const Window& window) const noexcept override {}
    [[nodiscard]] bool WasAnyControllerJustUsed() const noexcept override {}
protected:
private:
};
