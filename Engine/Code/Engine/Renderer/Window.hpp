#pragma once

#include "Engine/Platform/Win.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/RHI/RHITypes.hpp"

#include <functional>
#include <string>

struct WindowDesc {
    IntVector2 position{};
    IntVector2 dimensions{1600, 900};
    RHIOutputMode mode = RHIOutputMode::Windowed;
};

class Window {
public:
    Window() noexcept;
    explicit Window(const IntVector2& position, const IntVector2& dimensions) noexcept;
    explicit Window(const WindowDesc& desc) noexcept;
    ~Window() noexcept;

    void Open() noexcept;
    void Close() noexcept;

    void Show() noexcept;
    void Hide() noexcept;
    void UnHide() noexcept;
    [[nodiscard]] bool IsOpen() const noexcept;
    [[nodiscard]] bool IsClosed() const noexcept;
    [[nodiscard]] bool IsWindowed() const noexcept;
    [[nodiscard]] bool IsFullscreen() const noexcept;

    [[nodiscard]] IntVector2 GetDimensions() const noexcept;
    [[nodiscard]] IntVector2 GetClientDimensions() const noexcept;
    [[nodiscard]] IntVector2 GetPosition() const noexcept;

    [[nodiscard]] static IntVector2 GetDesktopResolution() noexcept;

    void SetDimensionsAndPosition(const IntVector2& new_position, const IntVector2& new_size) noexcept;
    void SetPosition(const IntVector2& new_position) noexcept;
    void SetDimensions(const IntVector2& new_dimensions) noexcept;
    void SetForegroundWindow() noexcept;
    void SetFocus() noexcept;

    [[nodiscard]] void* GetWindowHandle() const noexcept;
    void SetWindowHandle(void* hWnd) noexcept;

    [[nodiscard]] HDC GetWindowDeviceContext() const noexcept;

    [[nodiscard]] const RHIOutputMode& GetDisplayMode() const noexcept;
    void SetDisplayMode(const RHIOutputMode& display_mode) noexcept;

    std::function<bool(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)> custom_message_handler;

    void SetTitle(const std::string& title) noexcept;
    [[nodiscard]] const std::string& GetTitle() const noexcept;

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
    static inline constexpr unsigned long _defaultWindowedStyleFlags{WS_CAPTION | WS_SIZEBOX | WS_MINIMIZEBOX | WS_BORDER | WS_SYSMENU | WS_OVERLAPPED};
    static inline constexpr unsigned long _defaultBorderlessStyleFlags{WS_POPUP};
    static inline constexpr unsigned long _defaultStyleFlagsEx{WS_EX_APPWINDOW | WS_EX_ACCEPTFILES};
};
