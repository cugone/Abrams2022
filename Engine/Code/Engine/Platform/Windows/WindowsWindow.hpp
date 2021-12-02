#pragma once

#include "Engine/Math/IntVector2.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/RHI/RHITypes.hpp"

#include "Engine/Renderer/Window.hpp"

#include <functional>
#include <string>


class WindowsWindow : public Window {
public:
    WindowsWindow() noexcept;
    explicit WindowsWindow(const IntVector2& position, const IntVector2& dimensions) noexcept;
    explicit WindowsWindow(const WindowDesc& desc) noexcept;
    virtual ~WindowsWindow() noexcept;

    void Open() noexcept override;
    virtual void Close() noexcept override;

    void Show() noexcept override;
    void Hide() noexcept override;
    void UnHide() noexcept override;
    [[nodiscard]] bool IsOpen() const noexcept override;
    [[nodiscard]] bool IsClosed() const noexcept override;
    [[nodiscard]] bool IsWindowed() const noexcept override;
    [[nodiscard]] bool IsFullscreen() const noexcept override;

    [[nodiscard]] IntVector2 GetDimensions() const noexcept override;
    [[nodiscard]] IntVector2 GetClientDimensions() const noexcept override;
    [[nodiscard]] IntVector2 GetPosition() const noexcept override;

    void SetDimensionsAndPosition(const IntVector2& new_position, const IntVector2& new_size) noexcept override;
    void SetPosition(const IntVector2& new_position) noexcept override;
    void SetDimensions(const IntVector2& new_dimensions) noexcept override;
    void SetForegroundWindow() noexcept override;
    void SetFocus() noexcept override;

    [[nodiscard]] void* GetWindowHandle() const noexcept override;
    void SetWindowHandle(void* hWnd) noexcept override;

    [[nodiscard]] void* GetWindowDeviceContext() const noexcept override;

    [[nodiscard]] const RHIOutputMode& GetDisplayMode() const noexcept override;
    void SetDisplayMode(const RHIOutputMode& display_mode) noexcept override;

    std::function<bool(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)> custom_message_handler;
    void SetTitle(const std::string& title) noexcept override;
    [[nodiscard]] const std::string& GetTitle() const noexcept override;

protected:
    [[nodiscard]] bool Register() noexcept;
    [[nodiscard]] bool Unregister() noexcept;
    [[nodiscard]] bool Create() noexcept;

private:
    RHIOutputMode _currentDisplayMode = RHIOutputMode::Windowed;
    HWND _hWnd{};
    HDC _hdc{};
    HINSTANCE _hInstance{};
    std::string _title{"Created with Abrams 2022 (c) Casey Ugone "};
    INT _cmdShow{};
    WNDCLASSEX _wc{};
    int _positionX{};
    int _positionY{};
    unsigned int _width{1600u};
    unsigned int _height{900u};
    unsigned int _oldclientWidth{1600u};
    unsigned int _oldclientHeight{900u};
    unsigned int _clientWidth{1600u};
    unsigned int _clientHeight{900u};
    unsigned long _styleFlags{};
    unsigned long _styleFlagsEx{};
    bool _hasMenu{};
    static inline std::size_t _refCount{0u};
    static inline constexpr unsigned long _defaultWindowedStyleFlags{WS_OVERLAPPEDWINDOW | WS_BORDER /* WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */};
    static inline constexpr unsigned long _defaultBorderlessStyleFlags{WS_POPUP};
    static inline constexpr unsigned long _defaultStyleFlagsEx{WS_EX_APPWINDOW | WS_EX_ACCEPTFILES};
};
