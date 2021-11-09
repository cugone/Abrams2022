#pragma once
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Input/KeyCode.hpp"
#include "Engine/Input/XboxController.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"

#include "Engine/Services/IInputService.hpp"

#include <array>
#include <bitset>

class FileLogger;
class Renderer;
class Window;

class InputSystem : public EngineSubsystem, public IInputService {
public:
    InputSystem() noexcept;
    InputSystem(const InputSystem& other) noexcept = delete;
    InputSystem(InputSystem&& r_other) noexcept = delete;
    InputSystem& operator=(const InputSystem& rhs) noexcept = delete;
    InputSystem& operator=(InputSystem&& rhs) noexcept = delete;
    virtual ~InputSystem() noexcept;

    [[nodiscard]] virtual bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;
    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    [[nodiscard]] bool WasAnyKeyPressed() const noexcept override;
    [[nodiscard]] bool WasAnyMouseButtonPressed() const noexcept override;
    [[nodiscard]] bool WasMouseMoved() const noexcept override;
    [[nodiscard]] bool WasMouseJustUsed() const noexcept override;
    [[nodiscard]] bool IsKeyUp(const KeyCode& key) const noexcept override;
    [[nodiscard]] bool WasKeyJustPressed(const KeyCode& key) const noexcept override;
    [[nodiscard]] bool IsKeyDown(const KeyCode& key) const noexcept override;
    [[nodiscard]] bool WasKeyJustPressedOrIsKeyDown(const KeyCode& key) const noexcept override;
    [[nodiscard]] bool IsAnyKeyDown() const noexcept override;
    [[nodiscard]] bool WasKeyJustReleased(const KeyCode& key) const noexcept override;
    [[nodiscard]] bool WasMouseWheelJustScrolledUp() const noexcept override;
    [[nodiscard]] bool WasMouseWheelJustScrolledDown() const noexcept override;
    [[nodiscard]] bool WasMouseWheelJustScrolledLeft() const noexcept override;
    [[nodiscard]] bool WasMouseWheelJustScrolledRight() const noexcept override;
    [[nodiscard]] bool WasMouseWheelJustUsed() const noexcept override;

    [[nodiscard]] std::size_t GetConnectedControllerCount() const noexcept override;
    [[nodiscard]] bool IsAnyControllerConnected() const noexcept override;
    [[nodiscard]] const XboxController& GetXboxController(const std::size_t& controllerIndex) const noexcept override;
    [[nodiscard]] XboxController& GetXboxController(const std::size_t& controllerIndex) noexcept override;

    [[nodiscard]] bool IsMouseCursorVisible() const noexcept override;
    void ToggleMouseCursorVisibility() noexcept override;
    void HideMouseCursor() noexcept override;
    void ShowMouseCursor() noexcept override;

    void SetCursorScreenPosition(const Vector2& screen_pos) noexcept override;
    [[nodiscard]] Vector2 GetCursorScreenPosition() const noexcept override;

    void SetCursorWindowPosition(const Window& window, const Vector2& window_pos) noexcept override;
    [[nodiscard]] Vector2 GetCursorWindowPosition(const Window& window_ref) const noexcept override;

    void SetCursorWindowPosition(const Vector2& window_pos) noexcept override;
    [[nodiscard]] Vector2 GetCursorWindowPosition() noexcept override;

    void SetCursorToScreenCenter() noexcept override;
    void SetCursorToWindowCenter(const Window& window_ref) noexcept override;
    void SetCursorToWindowCenter() noexcept override;

    [[nodiscard]] Vector2 GetMouseDeltaFromWindowCenter() const noexcept override;
    [[nodiscard]] Vector2 GetMouseDeltaFromWindowCenter(const Window& window_ref) const noexcept override;
    [[nodiscard]] const Vector2& GetMouseCoords() const noexcept override;
    [[nodiscard]] const Vector2& GetMouseDelta() const noexcept override;

    [[nodiscard]] int GetMouseWheelPosition() const noexcept override;
    [[nodiscard]] int GetMouseWheelPositionNormalized() const noexcept override;

    [[nodiscard]] int GetMouseWheelHorizontalPosition() const noexcept override;
    [[nodiscard]] int GetMouseWheelHorizontalPositionNormalized() const noexcept override;

    [[nodiscard]] IntVector2 GetMouseWheelPositionAsIntVector2() const noexcept override;

    [[nodiscard]] bool IsMouseLockedToViewport() const noexcept override;
    void LockMouseToViewport(const Window& window) const noexcept override;
    void LockMouseToWindowViewport() const noexcept override;
    void UnlockMouseFromViewport() const noexcept override;

    [[nodiscard]] Vector2 GetScreenCenter() const noexcept override;
    [[nodiscard]] Vector2 GetWindowCenter() const noexcept override;
    [[nodiscard]] Vector2 GetWindowCenter(const Window& window) const noexcept override;
    [[nodiscard]] bool WasAnyControllerJustUsed() const noexcept override;

protected:
private:
    void RegisterKeyDown(unsigned char keyIndex) noexcept;
    void RegisterKeyUp(unsigned char keyIndex) noexcept;

    void UpdateXboxConnectedState() noexcept;

    void SetMouseCoords(float newX, float newY) noexcept;
    void SetMouseCoords(Vector2 newCoords) noexcept;
    void UpdateMouseCoords(float newX, float newY) noexcept;
    void UpdateMouseCoords(Vector2 newCoords) noexcept;
    void AdjustMouseCoords(float offsetX, float offsetY) noexcept;
    void AdjustMouseCoords(Vector2 offset) noexcept;

    std::array<XboxController, 4> _xboxControllers{};
    std::bitset<(std::size_t)KeyCode::Max> _previousKeys{};
    std::bitset<(std::size_t)KeyCode::Max> _currentKeys{};
    Vector2 _mouseCoords = Vector2::Zero;
    Vector2 _mousePrevCoords = Vector2::Zero;
    Vector2 _mouseDelta = Vector2::Zero;
    mutable AABB2 _initialClippingArea{};
    mutable AABB2 _currentClippingArea{};
    Stopwatch _connection_poll = Stopwatch(TimeUtils::FPSeconds{1.0f});
    int _mouseWheelPosition = 0;
    int _mouseWheelHPosition = 0;
    const int _max_controller_count = 4;
    bool _cursor_visible = true;
    mutable bool _should_clip_cursor = false;
};
