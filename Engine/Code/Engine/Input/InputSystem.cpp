#include "Engine/Input/InputSystem.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/RHI/RHIOutput.hpp"

#include "Engine/Renderer/Window.hpp"


#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#include <algorithm>
#include <sstream>

#include <hidusage.h>

Vector2 InputSystem::GetCursorWindowPosition(const Window& window_ref) const noexcept {
    POINT p;
    if(::GetCursorPos(&p)) {
        if(::ScreenToClient(static_cast<HWND>(window_ref.GetWindowHandle()), &p)) {
            return Vector2{static_cast<float>(p.x), static_cast<float>(p.y)};
        }
    }
    return Vector2::Zero;
}

Vector2 InputSystem::GetCursorWindowPosition() noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    return GetCursorWindowPosition(*window);
}

Vector2 InputSystem::GetCursorScreenPosition() const noexcept {
    POINT p;
    if(::GetCursorPos(&p)) {
        return Vector2{static_cast<float>(p.x), static_cast<float>(p.y)};
    }
    return Vector2::Zero;
}

void InputSystem::SetCursorToScreenCenter() noexcept {
    HWND desktop_window = ::GetDesktopWindow();
    RECT desktop_client;
    if(::GetClientRect(desktop_window, &desktop_client)) {
        const auto center_x = desktop_client.left + (desktop_client.right - desktop_client.left) * 0.5f;
        const auto center_y = desktop_client.top + (desktop_client.bottom - desktop_client.top) * 0.5f;
        SetCursorScreenPosition(Vector2{center_x, center_y});
    }
}

void InputSystem::SetCursorToWindowCenter(const Window& window_ref) noexcept {
    RECT client_area;
    if(::GetClientRect(static_cast<HWND>(window_ref.GetWindowHandle()), &client_area)) {
        const auto center_x = client_area.left + (client_area.right - client_area.left) * 0.5f;
        const auto center_y = client_area.top + (client_area.bottom - client_area.top) * 0.5f;
        SetCursorWindowPosition(window_ref, Vector2{center_x, center_y});
    }
}

void InputSystem::SetCursorToWindowCenter() noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    SetCursorToWindowCenter(*window);
}

Vector2 InputSystem::GetMouseDeltaFromWindowCenter() const noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    return GetMouseDeltaFromWindowCenter(*window);
}

Vector2 InputSystem::GetMouseDeltaFromWindowCenter(const Window& window_ref) const noexcept {
    return GetMouseCoords() - GetWindowCenter(window_ref);
}

void InputSystem::SetCursorScreenPosition(const Vector2& screen_pos) noexcept {
    const auto x = static_cast<int>(screen_pos.x);
    const auto y = static_cast<int>(screen_pos.y);
    ::SetCursorPos(x, y);
    SetMouseCoords(static_cast<float>(x), static_cast<float>(y));
}

void InputSystem::UpdateXboxConnectedState() noexcept {
    for(int i = 0; i < m_max_controller_count; ++i) {
        m_xboxControllers[i].UpdateConnectedState(i);
    }
}

void InputSystem::SetMouseCoords(float newX, float newY) noexcept {
    SetMouseCoords(Vector2{newX, newY});
}

void InputSystem::SetMouseCoords(Vector2 newCoords) noexcept {
    m_mouseCoords = newCoords;
}

void InputSystem::UpdateMouseCoords(float newX, float newY) noexcept {
    UpdateMouseCoords(Vector2{newX, newY});
}

void InputSystem::UpdateMouseCoords(Vector2 newCoords) noexcept {
    m_mousePrevCoords = m_mouseCoords;
    m_mouseCoords = newCoords;
    m_mouseDelta = m_mouseCoords - m_mousePrevCoords;
}

void InputSystem::AdjustMouseCoords(float offsetX, float offsetY) noexcept {
    AdjustMouseCoords(Vector2{offsetX, offsetY});
}

void InputSystem::AdjustMouseCoords(Vector2 offset) noexcept {
    m_mousePrevCoords = m_mouseCoords;
    m_mouseCoords += offset;
    m_mouseDelta = m_mouseCoords - m_mousePrevCoords;
}

bool InputSystem::WasMouseWheelJustUsed() const noexcept {
    return GetMouseWheelPositionNormalized() != 0 && GetMouseWheelHorizontalPositionNormalized() != 0;
}

Vector2 InputSystem::GetScreenCenter() const noexcept {
    RECT desktopRect;
    HWND desktopWindowHandle = ::GetDesktopWindow();
    if(::GetClientRect(desktopWindowHandle, &desktopRect)) {
        const auto center_x = desktopRect.left + (desktopRect.right - desktopRect.left) * 0.5f;
        const auto center_y = desktopRect.top + (desktopRect.bottom - desktopRect.top) * 0.5f;
        return Vector2{center_x, center_y};
    }
    return Vector2::Zero;
}

Vector2 InputSystem::GetWindowCenter() const noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    return GetWindowCenter(*window);
}

Vector2 InputSystem::GetWindowCenter(const Window& window) const noexcept {
    RECT rect;
    HWND windowHandle = static_cast<HWND>(window.GetWindowHandle());
    if(::GetClientRect(windowHandle, &rect)) {
        const auto center_x = rect.left + (rect.right - rect.left) * 0.50f;
        const auto center_y = rect.top + (rect.bottom - rect.top) * 0.50f;
        return Vector2{center_x, center_y};
    }
    return Vector2::Zero;
}

bool InputSystem::WasAnyControllerJustUsed() const noexcept {
    bool result = false;
    for(int i = 0; i < m_max_controller_count; ++i) {
        result |= m_xboxControllers[i].IsAnyButtonDown();
        result |= m_xboxControllers[i].GetLeftThumbPosition().CalcLengthSquared() > 0.0f;
        result |= m_xboxControllers[i].GetRightThumbPosition().CalcLengthSquared() > 0.0f;
        result |= m_xboxControllers[i].GetLeftTriggerPosition() > 0.0f;
        result |= m_xboxControllers[i].GetRightTriggerPosition() > 0.0f;
        if(result)
            break;
    }
    return result;
}

bool InputSystem::IsMouseCursorVisible() const noexcept {
    CURSORINFO info{};
    info.cbSize = sizeof(CURSORINFO);
    ::GetCursorInfo(&info);
    return info.flags != 0;
}

void InputSystem::HideMouseCursor() noexcept {
    while(::ShowCursor(FALSE) >= 0)
        ;
    m_cursor_visible = IsMouseCursorVisible();
}

void InputSystem::ShowMouseCursor() noexcept {
    while(::ShowCursor(TRUE) < 0)
        ;
    m_cursor_visible = IsMouseCursorVisible();
}

void InputSystem::ToggleMouseCursorVisibility() noexcept {
    IsMouseCursorVisible() ? HideMouseCursor() : ShowMouseCursor();
}

void InputSystem::SetCursorWindowPosition(const Window& window, const Vector2& window_pos) noexcept {
    POINT p{};
    p.x = static_cast<long>(window_pos.x);
    p.y = static_cast<long>(window_pos.y);
    if(::ClientToScreen(static_cast<HWND>(window.GetWindowHandle()), &p)) {
        SetCursorScreenPosition(Vector2{static_cast<float>(p.x), static_cast<float>(p.y)});
    }
}

void InputSystem::SetCursorWindowPosition(const Vector2& window_pos) noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    SetCursorWindowPosition(*window, window_pos);
}

Vector2 InputSystem::GetMouseCoords() const noexcept {
    return m_mouseCoords;
}

Vector2 InputSystem::GetMouseDelta() const noexcept {
    return m_mouseDelta;
}

int InputSystem::GetMouseWheelPosition() const noexcept {
    return m_mouseWheelPosition;
}

int InputSystem::GetMouseWheelPositionNormalized() const noexcept {
    if(m_mouseWheelPosition) {
        return m_mouseWheelPosition / std::abs(m_mouseWheelPosition);
    }
    return 0;
}

int InputSystem::GetMouseWheelHorizontalPosition() const noexcept {
    return m_mouseWheelHPosition;
}

int InputSystem::GetMouseWheelHorizontalPositionNormalized() const noexcept {
    if(m_mouseWheelHPosition) {
        return m_mouseWheelHPosition / std::abs(m_mouseWheelHPosition);
    }
    return 0;
}

IntVector2 InputSystem::GetMouseWheelPositionAsIntVector2() const noexcept {
    return IntVector2{m_mouseWheelHPosition, m_mouseWheelPosition};
}

bool InputSystem::IsMouseLockedToViewport() const noexcept {
    return m_should_clip_cursor;
}

void InputSystem::LockMouseToViewport(const Window& window) const noexcept {
    const auto dims = window.GetDimensions();
    const auto pos = window.GetPosition();
    const long top = pos.x;
    const long left = pos.x;
    const long right = pos.x + dims.x;
    const long bottom = pos.y + dims.y;
    RECT temp{top, left, right, bottom};
    if(::ClipCursor(&temp)) {
        m_should_clip_cursor = true;
        m_currentClippingArea = RectToAABB2(temp);
    }
}

void InputSystem::LockMouseToWindowViewport() const noexcept {
    const auto* const window = ServiceLocator::const_get<IRendererService>()->GetOutput()->GetWindow();
    LockMouseToViewport(*window);
}

void InputSystem::UnlockMouseFromViewport() const noexcept {
    ::ClipCursor(nullptr);
    m_should_clip_cursor = false;
    m_currentClippingArea = m_initialClippingArea;
}

void InputSystem::RegisterKeyDown(unsigned char keyIndex) noexcept {
    auto kc = ConvertWinVKToKeyCode(keyIndex);
    m_currentKeys[(std::size_t)kc] = true;
}

void InputSystem::RegisterKeyUp(unsigned char keyIndex) noexcept {
    auto kc = ConvertWinVKToKeyCode(keyIndex);
    m_currentKeys[(std::size_t)kc] = false;
}

bool InputSystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    switch(msg.wmMessageCode) {
    case WindowsSystemMessage::App_DeviceChanged: {
        UpdateXboxConnectedState();
        return false;
    }
    case WindowsSystemMessage::Keyboard_Help: {
        //TODO: Event system should push a "Help" event.
        return false;
    }
    case WindowsSystemMessage::Keyboard_KeyDown: {
        LPARAM lp = msg.lparam;
        WPARAM wp = msg.wparam;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 0 for KEYDOWN
        //P: previous state 1 for already down
        //T: transition state 0 for KEYDOWN
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        constexpr uint16_t keystate_state_mask = 0b1000'0000'0000'0000;                       //0x8000
        //constexpr uint16_t keystate_toggle_mask = 0b0000'0000'0000'0001;                      //0x0001
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        auto my_key = ConvertWinVKToKeyCode(key);
        if(my_key == KeyCode::Unknown) {
            return true;
        }
        if(is_extended_key) {
            switch(my_key) {
            case KeyCode::Shift: {
                auto left_key = !!(::GetKeyState(VK_LSHIFT) & keystate_state_mask);
                auto right_key = !!(::GetKeyState(VK_RSHIFT) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LSHIFT);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RSHIFT);
                auto shift_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyDown(shift_key);
                }
                break;
            }
            case KeyCode::Alt: {
                auto left_key = !!(::GetKeyState(VK_LMENU) & keystate_state_mask);
                auto right_key = !!(::GetKeyState(VK_RMENU) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LMENU);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RMENU);
                auto alt_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyDown(alt_key);
                }
                break;
            }
            case KeyCode::Ctrl: {
                auto left_key = !!(::GetKeyState(VK_LCONTROL) & keystate_state_mask);
                auto right_key = !!(::GetKeyState(VK_RCONTROL) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LCONTROL);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RCONTROL);
                auto ctrl_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyDown(ctrl_key);
                }
                break;
            }
            case KeyCode::Return: my_key = KeyCode::NumPadEnter; break;
            case KeyCode::LWin: {
                auto left_key = !!(::GetKeyState(VK_LWIN) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LWIN);
                my_key = left_key ? my_leftkey : KeyCode::Unknown;
                break;
            }
            case KeyCode::RWin: {
                auto right_key = !!(::GetKeyState(VK_RWIN) & keystate_state_mask);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RWIN);
                my_key = right_key ? my_rightkey : KeyCode::Unknown;
                break;
            }
            }
        }
        switch(my_key) {
        case KeyCode::Shift: {
            auto left_key = !!(::GetKeyState(VK_LSHIFT) & keystate_state_mask);
            auto right_key = !!(::GetKeyState(VK_RSHIFT) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LSHIFT);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RSHIFT);
            auto shift_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyDown(shift_key);
            }
            break;
        }
        case KeyCode::Ctrl: {
            auto left_key = !!(::GetKeyState(VK_LCONTROL) & keystate_state_mask);
            auto right_key = !!(::GetKeyState(VK_RCONTROL) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LCONTROL);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RCONTROL);
            auto ctrl_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyDown(ctrl_key);
            }
            break;
        }
        case KeyCode::Alt: {
            auto left_key = !!(::GetKeyState(VK_LMENU) & keystate_state_mask);
            auto right_key = !!(::GetKeyState(VK_RMENU) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LMENU);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RMENU);
            auto alt_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyDown(alt_key);
            }
            break;
        }
        }
        key = ConvertKeyCodeToWinVK(my_key);
        RegisterKeyDown(key);
        return true;
    }
    case WindowsSystemMessage::Keyboard_KeyUp: {
        LPARAM lp = msg.lparam;
        WPARAM wp = msg.wparam;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 0 for KEYUP
        //P: previous state 1 for already down
        //T: transition state 1 for KEYUP
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        constexpr uint16_t keystate_state_mask = 0b1000'0000'0000'0000;                       //0x8000
        //constexpr uint16_t keystate_toggle_mask = 0b0000'0000'0000'0001;                      //0x0001
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        auto my_key = ConvertWinVKToKeyCode(key);
        if(my_key == KeyCode::Unknown) {
            return true;
        }
        if(is_extended_key) {
            switch(my_key) {
            case KeyCode::Shift: {
                auto left_down = IsKeyDown(KeyCode::LShift);
                auto right_down = IsKeyDown(KeyCode::RShift);
                auto left_key = left_down && !!!(::GetKeyState(VK_LSHIFT) & keystate_state_mask);
                auto right_key = right_down && !!!(::GetKeyState(VK_RSHIFT) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LSHIFT);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RSHIFT);
                auto shift_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyUp(shift_key);
                }
                break;
            }
            case KeyCode::Alt: {
                auto left_down = IsKeyDown(KeyCode::LAlt);
                auto right_down = IsKeyDown(KeyCode::RAlt);
                auto left_key = left_down && !!!(::GetKeyState(VK_LMENU) & keystate_state_mask);
                auto right_key = right_down && !!!(::GetKeyState(VK_RMENU) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LMENU);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RMENU);
                auto alt_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyUp(alt_key);
                }
                break;
            }
            case KeyCode::Ctrl: {
                auto left_down = IsKeyDown(KeyCode::LControl);
                auto right_down = IsKeyDown(KeyCode::RControl);
                auto left_key = left_down && !!!(::GetKeyState(VK_LCONTROL) & keystate_state_mask);
                auto right_key = right_down && !!!(::GetKeyState(VK_RCONTROL) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LCONTROL);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RCONTROL);
                auto ctrl_key = ConvertKeyCodeToWinVK(my_key);
                my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyUp(ctrl_key);
                }
                break;
            }
            case KeyCode::Return: my_key = KeyCode::NumPadEnter; break;
            case KeyCode::LWin: {
                auto left_down = IsKeyDown(KeyCode::LWin);
                auto left_key = left_down && !!!(::GetKeyState(VK_LWIN) & keystate_state_mask);
                auto my_leftkey = ConvertWinVKToKeyCode(VK_LWIN);
                my_key = left_key ? my_leftkey : KeyCode::Unknown;
                break;
            }
            case KeyCode::RWin: {
                auto right_down = IsKeyDown(KeyCode::RWin);
                auto right_key = right_down && !!!(::GetKeyState(VK_RWIN) & keystate_state_mask);
                auto my_rightkey = ConvertWinVKToKeyCode(VK_RWIN);
                my_key = right_key ? my_rightkey : KeyCode::Unknown;
                break;
            }
            }
        }
        switch(my_key) {
        case KeyCode::Shift: {
            auto left_down = IsKeyDown(KeyCode::LShift);
            auto right_down = IsKeyDown(KeyCode::RShift);
            auto left_key = left_down && !!!(::GetKeyState(VK_LSHIFT) & keystate_state_mask);
            auto right_key = right_down && !!!(::GetKeyState(VK_RSHIFT) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LSHIFT);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RSHIFT);
            auto shift_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyUp(shift_key);
            }
            break;
        }
        case KeyCode::Ctrl: {
            auto left_down = IsKeyDown(KeyCode::LControl);
            auto right_down = IsKeyDown(KeyCode::RControl);
            auto left_key = left_down && !!!(::GetKeyState(VK_LCONTROL) & keystate_state_mask);
            auto right_key = right_down && !!!(::GetKeyState(VK_RCONTROL) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LCONTROL);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RCONTROL);
            auto ctrl_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyUp(ctrl_key);
            }
            break;
        }
        case KeyCode::Alt: {
            auto left_down = IsKeyDown(KeyCode::LAlt);
            auto right_down = IsKeyDown(KeyCode::RAlt);
            auto left_key = left_down && !!!(::GetKeyState(VK_LMENU) & keystate_state_mask);
            auto right_key = right_down && !!!(::GetKeyState(VK_RMENU) & keystate_state_mask);
            auto my_leftkey = ConvertWinVKToKeyCode(VK_LMENU);
            auto my_rightkey = ConvertWinVKToKeyCode(VK_RMENU);
            auto alt_key = ConvertKeyCodeToWinVK(my_key);
            my_key = left_key ? my_leftkey : (right_key ? my_rightkey : KeyCode::Unknown);
            if(my_key != KeyCode::Unknown) {
                RegisterKeyUp(alt_key);
            }
            break;
        }
        }
        key = ConvertKeyCodeToWinVK(my_key);
        RegisterKeyUp(key);
        return true;
    }
    case WindowsSystemMessage::Keyboard_SysKeyDown: {
        LPARAM lp = msg.lparam;
        WPARAM wp = msg.wparam;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 1 for Alt while key pressed; 0 posted to active window if no window has keyboard focus
        //P: previous state 1 for already down
        //T: transition state 0 for SYSKEYDOWN
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        constexpr uint16_t keystate_state_mask = 0b1000'0000'0000'0000;                       //0x8000
        //constexpr uint16_t keystate_toggle_mask = 0b0000'0000'0000'0001;                      //0x0001
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        auto my_key = ConvertWinVKToKeyCode(key);
        if(my_key == KeyCode::Unknown) {
            return true;
        }
        if(is_extended_key) {
            switch(my_key) {
            case KeyCode::Alt: //Right Alt
            {
                auto is_key_down = !!(::GetKeyState(key) & keystate_state_mask);
                auto my_rightkey = KeyCode::RAlt;
                auto alt_key = ConvertKeyCodeToWinVK(KeyCode::Alt);
                my_key = is_key_down ? my_rightkey : KeyCode::Unknown;
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyDown(alt_key);
                }
                break;
            }
            }
        }
        switch(my_key) {
        case KeyCode::Alt: //Left Alt
        {
            auto is_key_down = !!(::GetKeyState(VK_LMENU) & keystate_state_mask);
            auto my_leftkey = KeyCode::LAlt;
            auto alt_key = ConvertKeyCodeToWinVK(KeyCode::Alt);
            my_key = is_key_down ? my_leftkey : KeyCode::Unknown;
            if(my_key != KeyCode::Unknown) {
                RegisterKeyDown(alt_key);
            }
            break;
        }
        case KeyCode::F10: {
            auto is_key_down = !!(::GetKeyState(VK_F10) & keystate_state_mask);
            my_key = is_key_down ? KeyCode::F10 : KeyCode::Unknown;
        }
        }
        key = ConvertKeyCodeToWinVK(my_key);
        RegisterKeyDown(key);
        return true;
    }
    case WindowsSystemMessage::Keyboard_SysKeyUp: {
        LPARAM lp = msg.lparam;
        WPARAM wp = msg.wparam;
        auto key = static_cast<unsigned char>(wp);
        auto lpBits = static_cast<uint32_t>(lp & 0xFFFFFFFFu);
        //0bTPXRRRRESSSSSSSSCCCCCCCCCCCCCCCC
        //C: repeat count
        //S: scan code
        //E: extended key flag
        //R: reserved
        //X: context code: 1 for Alt down while released; 0 if SYSKEYDOWN posted to active window because no window has keyboard focus.
        //P: previous state 1 for SYSKEYUP
        //T: transition state 1 for SYSKEYUP
        //constexpr uint32_t repeat_count_mask = 0b0000'0000'0000'0000'1111'1111'1111'1111;     //0x0000FFFF;
        //constexpr uint32_t scan_code_mask = 0b0000'0000'1111'1111'0000'0000'0000'0000;        //0x00FF0000;
        constexpr uint32_t extended_key_mask = 0b0000'0001'0000'0000'0000'0000'0000'0000;     //0x01000000;
        //constexpr uint32_t reserved_mask = 0b0001'1110'0000'0000'0000'0000'0000'0000;         //0x1E000000;
        //constexpr uint32_t context_code_mask = 0b0010'0000'0000'0000'0000'0000'0000'0000;     //0x20000000;
        //constexpr uint32_t previous_state_mask = 0b0100'0000'0000'0000'0000'0000'0000'0000;   //0x40000000;
        //constexpr uint32_t transition_state_mask = 0b1000'0000'0000'0000'0000'0000'0000'0000; //0x80000000;
        constexpr uint16_t keystate_state_mask = 0b1000'0000'0000'0000;                       //0x8000
        //constexpr uint16_t keystate_toggle_mask = 0b0000'0000'0000'0001;                      //0x0001
        bool is_extended_key = (lpBits & extended_key_mask) != 0;
        auto my_key = ConvertWinVKToKeyCode(key);
        if(my_key == KeyCode::Unknown) {
            return true;
        }
        if(is_extended_key) {
            switch(my_key) {
            case KeyCode::Alt: //Right Alt
            {
                auto is_key_up = !(!!(::GetKeyState(VK_RMENU) & keystate_state_mask));
                auto my_rightkey = KeyCode::RAlt;
                auto alt_key = ConvertKeyCodeToWinVK(KeyCode::Alt);
                my_key = is_key_up ? my_rightkey : KeyCode::Unknown;
                if(my_key != KeyCode::Unknown) {
                    RegisterKeyUp(alt_key);
                }
                break;
            }
            }
        }
        switch(my_key) {
        case KeyCode::Alt: //Left Alt
        {
            auto is_key_up = !(!!(::GetKeyState(VK_LMENU) & keystate_state_mask));
            auto my_leftkey = KeyCode::LAlt;
            auto alt_key = ConvertKeyCodeToWinVK(KeyCode::Alt);
            my_key = is_key_up ? my_leftkey : KeyCode::Unknown;
            if(my_key != KeyCode::Unknown) {
                RegisterKeyUp(alt_key);
            }
            break;
        }
        case KeyCode::F10: {
            auto is_key_up = !(!!(::GetKeyState(VK_F10) & keystate_state_mask));
            my_key = is_key_up ? KeyCode::F10 : KeyCode::Unknown;
        }
        }
        key = ConvertKeyCodeToWinVK(my_key);
        RegisterKeyUp(key);
        return true;
    }
    case WindowsSystemMessage::Mouse_LButtonDown: {
        constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(wp & lbutton_mask) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::LButton);
            RegisterKeyDown(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_LButtonUp: {
        constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(!(wp & lbutton_mask)) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::LButton);
            RegisterKeyUp(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_RButtonDown: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(wp & rbutton_mask) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::RButton);
            RegisterKeyDown(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_RButtonUp: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(!(wp & rbutton_mask)) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::RButton);
            RegisterKeyUp(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_MButtonDown: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(wp & mbutton_mask) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::MButton);
            RegisterKeyDown(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_MButtonUp: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;  //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;  //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;    //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;     //0x0008
        constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;  //0x0010
        //constexpr uint16_t xbutton1_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        if(!(wp & mbutton_mask)) {
            unsigned char key = ConvertKeyCodeToWinVK(KeyCode::MButton);
            RegisterKeyUp(key);
            const auto x = static_cast<float>(GET_X_LPARAM(lp));
            const auto y = static_cast<float>(GET_Y_LPARAM(lp));
            SetMouseCoords(x, y);
            return true;
        }
        return false;
    }
    case WindowsSystemMessage::Mouse_XButtonDown: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;       //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;       //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;         //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;          //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;       //0x0010
        //constexpr uint16_t xbutton1_down_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_down_mask = 0b0000'0000'0100'0000; //0x0040
        constexpr uint16_t xbutton1_mask = 0b0000'0001;                //0x0001
        constexpr uint16_t xbutton2_mask = 0b0000'0010;                //0x0002
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        auto buttons = GET_XBUTTON_WPARAM(wp);
        unsigned char key = ConvertKeyCodeToWinVK(KeyCode::XButton1);
        if(buttons & xbutton1_mask) {
            key = ConvertKeyCodeToWinVK(KeyCode::XButton1);
        }
        if(buttons & xbutton2_mask) {
            key = ConvertKeyCodeToWinVK(KeyCode::XButton2);
        }
        RegisterKeyDown(key);
        const auto x = static_cast<float>(GET_X_LPARAM(lp));
        const auto y = static_cast<float>(GET_Y_LPARAM(lp));
        SetMouseCoords(x, y);
        return true;
    }
    case WindowsSystemMessage::Mouse_XButtonUp: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;       //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;       //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;         //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;          //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;       //0x0010
        //constexpr uint16_t xbutton1_down_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_down_mask = 0b0000'0000'0100'0000; //0x0040
        constexpr uint16_t xbutton1_mask = 0b0000'0001;                //0x0001
        constexpr uint16_t xbutton2_mask = 0b0000'0010;                //0x0002
        WPARAM wp = msg.wparam;
        LPARAM lp = msg.lparam;
        auto buttons = GET_XBUTTON_WPARAM(wp);
        unsigned char key = 0;
        if(buttons & xbutton1_mask) {
            key = ConvertKeyCodeToWinVK(KeyCode::XButton1);
        }
        if(buttons & xbutton2_mask) {
            key = ConvertKeyCodeToWinVK(KeyCode::XButton2);
        }
        RegisterKeyUp(key);
        const auto x = static_cast<float>(GET_X_LPARAM(lp));
        const auto y = static_cast<float>(GET_Y_LPARAM(lp));
        SetMouseCoords(x, y);
        return true;
    }
    case WindowsSystemMessage::Mouse_MouseMove: {
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;       //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;       //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;         //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;          //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;       //0x0010
        //constexpr uint16_t xbutton1_down_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_down_mask = 0b0000'0000'0100'0000; //0x0040
        LPARAM lp = msg.lparam;
        const auto x = static_cast<float>(GET_X_LPARAM(lp));
        const auto y = static_cast<float>(GET_Y_LPARAM(lp));
        UpdateMouseCoords(x, y);
        return true;
    }
    case WindowsSystemMessage::Mouse_MouseWheel: {
        //constexpr uint16_t wheeldelta_mask = 0b1111'1111'0000'0000;    //FF00
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;       //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;       //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;         //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;          //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;       //0x0010
        //constexpr uint16_t xbutton1_down_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_down_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        m_mouseWheelPosition = GET_WHEEL_DELTA_WPARAM(wp);
        return true;
    }
    case WindowsSystemMessage::Mouse_MouseHWheel: {
        //constexpr uint16_t wheeldelta_mask = 0b1111'1111'0000'0000;    //FF00
        //constexpr uint16_t lbutton_mask = 0b0000'0000'0000'0001;       //0x0001
        //constexpr uint16_t rbutton_mask = 0b0000'0000'0000'0010;       //0x0002
        //constexpr uint16_t shift_mask = 0b0000'0000'0000'0100;         //0x0004
        //constexpr uint16_t ctrl_mask = 0b0000'0000'0000'1000;          //0x0008
        //constexpr uint16_t mbutton_mask = 0b0000'0000'0001'0000;       //0x0010
        //constexpr uint16_t xbutton1_down_mask = 0b0000'0000'0010'0000; //0x0020
        //constexpr uint16_t xbutton2_down_mask = 0b0000'0000'0100'0000; //0x0040
        WPARAM wp = msg.wparam;
        m_mouseWheelHPosition = GET_WHEEL_DELTA_WPARAM(wp);
        return true;
    }
    case WindowsSystemMessage::Window_Move: {
        if(!IsMouseLockedToViewport()) {
            return false;
        }
        LPARAM lp = msg.lparam;
        const auto width = m_currentClippingArea.CalcDimensions().x;
        const auto height = m_currentClippingArea.CalcDimensions().y;
        const auto x = static_cast<float>(LOWORD(lp));
        const auto y = static_cast<float>(HIWORD(lp));
        m_currentClippingArea.mins.y = y;
        m_currentClippingArea.mins.x = x;
        m_currentClippingArea.maxs.y = y + height;
        m_currentClippingArea.maxs.x = x + width;
        RECT result = AABB2ToRect(m_currentClippingArea);
        ::ClipCursor(&result);
        return true;
    }
    case WindowsSystemMessage::Window_Size: {
        if(!IsMouseLockedToViewport()) {
            return false;
        }
        const auto should_unclip = [msg]() -> const bool {
            const auto result = EngineSubsystem::GetResizeTypeFromWmSize(msg);
            return result == WindowResizeType::MaxHide || result == WindowResizeType::Minimized;
        }();
        if(should_unclip) {
            ::ClipCursor(nullptr);
            return false; //App needs to respond.
        }
        LPARAM lp = msg.lparam;
        const auto w = static_cast<float>(LOWORD(lp));
        const auto h = static_cast<float>(HIWORD(lp));
        const auto x = m_currentClippingArea.mins.x;
        const auto y = m_currentClippingArea.mins.y;
        m_currentClippingArea.mins.x   = x;
        m_currentClippingArea.mins.y   = y;
        m_currentClippingArea.maxs.x = x + w;
        m_currentClippingArea.maxs.y = y + h;
        RECT result = AABB2ToRect(m_currentClippingArea);
        ::ClipCursor(&result);
        return false; //App needs to respond
    }
    case WindowsSystemMessage::Window_ActivateApp: {
        if(!IsMouseLockedToViewport()) {
            return false;
        }
        WPARAM wp = msg.wparam;
        bool losing_focus = wp == FALSE;
        bool gaining_focus = wp == TRUE;
        if(losing_focus) {
            ::ClipCursor(nullptr);
        }
        if(gaining_focus) {
            RECT result = AABB2ToRect(m_currentClippingArea);
            ::ClipCursor(&result);
        }
        return false; //App needs to respond
    }
    case WindowsSystemMessage::Keyboard_Activate: {
        if(!IsMouseLockedToViewport()) {
            return false;
        }
        WPARAM wp = msg.wparam;
        auto active_type = LOWORD(wp);
        switch(active_type) {
        case WA_ACTIVE:
            [[fallthrough]];
        case WA_CLICKACTIVE:
            {
                RECT result = AABB2ToRect(m_currentClippingArea);
                ::ClipCursor(&result);
            }
            return false; //App needs to respond
        case WA_INACTIVE:
            ::ClipCursor(nullptr);
            return false; //App needs to respond
        default:
            return false; //App needs to respond
        }
    }
    }
    return false;
}

InputSystem::InputSystem() noexcept
: EngineSubsystem()
, IInputService()
{
    RECT result{};
    ::GetClipCursor(&result);
    m_initialClippingArea = RectToAABB2(result);
}

InputSystem::~InputSystem() noexcept {
    RECT result = AABB2ToRect(m_initialClippingArea);
    ::ClipCursor(&result);
}

void InputSystem::Initialize() noexcept {
    UpdateXboxConnectedState();
}

void InputSystem::BeginFrame() noexcept {
    if(m_connection_poll.CheckAndReset()) {
        UpdateXboxConnectedState();
    }
    for(int i = 0; i < m_max_controller_count; ++i) {
        m_xboxControllers[i].Update(i);
    }
}

void InputSystem::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    /* DO NOTHING */
}

void InputSystem::Render() const noexcept {
    /* DO NOTHING */
}

void InputSystem::EndFrame() noexcept {
    m_mouseDelta = Vector2::Zero;
    m_mousePrevCoords = m_mouseCoords;
    m_previousKeys = m_currentKeys;
    m_mouseWheelPosition = 0;
    m_mouseWheelHPosition = 0;
}

bool InputSystem::WasAnyKeyPressed() const noexcept {
    for(KeyCode k = KeyCode::First_; k < KeyCode::Last_; ++k) {
        if(WasKeyJustPressed(k)) {
            return true;
        }
    }
    return false;
}

bool InputSystem::WasAnyMouseButtonPressed() const noexcept {
    for(KeyCode k = KeyCode::FirstMouseButton_; k < KeyCode::LastMouseButton_; ++k) {
        if(WasKeyJustPressed(k)) {
            return true;
        }
    }
    return false;
}

bool InputSystem::WasMouseMoved() const noexcept {
    return GetMouseDelta().CalcLengthSquared() > 0.0f;
}

bool InputSystem::WasMouseJustUsed() const noexcept {
    return WasMouseMoved() || WasAnyMouseButtonPressed() || WasMouseWheelJustUsed();
}

bool InputSystem::IsKeyUp(const KeyCode& key) const noexcept {
    return !m_previousKeys[(std::size_t)key] && !m_currentKeys[(std::size_t)key];
}

bool InputSystem::WasKeyJustPressed(const KeyCode& key) const noexcept {
    return !m_previousKeys[(std::size_t)key] && m_currentKeys[(std::size_t)key];
}

bool InputSystem::IsKeyDown(const KeyCode& key) const noexcept {
    return m_previousKeys[(std::size_t)key] && m_currentKeys[(std::size_t)key];
}

bool InputSystem::WasKeyJustPressedOrIsKeyDown(const KeyCode& key) const noexcept {
    return WasKeyJustPressed(key) || IsKeyDown(key);
}

bool InputSystem::IsAnyKeyDown() const noexcept {
    for(KeyCode k = KeyCode::First_; k < KeyCode::Last_; ++k) {
        if(IsKeyDown(k)) {
            return true;
        }
    }
    return false;
}

bool InputSystem::WasKeyJustReleased(const KeyCode& key) const noexcept {
    return m_previousKeys[(std::size_t)key] && !m_currentKeys[(std::size_t)key];
}

bool InputSystem::WasMouseWheelJustScrolledUp() const noexcept {
    return GetMouseWheelPositionNormalized() > 0;
}

bool InputSystem::WasMouseWheelJustScrolledDown() const noexcept {
    return GetMouseWheelPositionNormalized() < 0;
}

bool InputSystem::WasMouseWheelJustScrolledLeft() const noexcept {
    return GetMouseWheelHorizontalPositionNormalized() < 0;
}

bool InputSystem::WasMouseWheelJustScrolledRight() const noexcept {
    return GetMouseWheelHorizontalPositionNormalized() > 0;
}

std::size_t InputSystem::GetConnectedControllerCount() const noexcept {
    int connected_count = 0;
    for(const auto& controller : m_xboxControllers) {
        if(controller.IsConnected()) {
            ++connected_count;
        }
    }
    return connected_count;
}

bool InputSystem::IsAnyControllerConnected() const noexcept {
    bool result = false;
    for(const auto& controller : m_xboxControllers) {
        result |= controller.IsConnected();
    }
    return result;
}

const XboxController& InputSystem::GetXboxController(const std::size_t& controllerIndex) const noexcept {
    return m_xboxControllers[controllerIndex];
}

XboxController& InputSystem::GetXboxController(const std::size_t& controllerIndex) noexcept {
    return m_xboxControllers[controllerIndex];
}
