#include "Engine/Renderer/Window.hpp"

#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/RHI/RHITypes.hpp"

#include "Engine/Core/FileUtils.hpp"

#include <algorithm>

Window::Window() noexcept
: _styleFlags{_defaultWindowedStyleFlags}
, _styleFlagsEx{_defaultStyleFlagsEx} {
    if(_refCount == 0) {
        if(Register()) {
            ++_refCount;
        }
    }

    RECT desktopRect;
    HWND desktopHwnd = ::GetDesktopWindow();
    ::GetClientRect(desktopHwnd, &desktopRect);
    ::AdjustWindowRectEx(&desktopRect, _styleFlags, _hasMenu, _styleFlagsEx);
}

Window::Window(const IntVector2& position, const IntVector2& dimensions) noexcept
: _styleFlags{_defaultWindowedStyleFlags}
, _styleFlagsEx{_defaultStyleFlagsEx} {
    if(_refCount == 0) {
        if(Register()) {
            ++_refCount;
        }
    }
    RECT r{};
    r.top = static_cast<long>(position.y);
    r.left = static_cast<long>(position.x);
    r.bottom = r.top + dimensions.y;
    r.right = r.left + dimensions.x;

    ::AdjustWindowRectEx(&r, _styleFlags, _hasMenu, _styleFlagsEx);

    _positionX = position.x - r.left;
    _positionY = position.y - r.top;
    _width = r.right - r.left;
    _height = r.bottom - r.top;
    _oldclientWidth = _clientWidth;
    _oldclientHeight = _clientHeight;
    _clientWidth = dimensions.x;
    _clientHeight = dimensions.y;
}

Window::Window(const WindowDesc& desc) noexcept
: _styleFlags{_defaultWindowedStyleFlags}
, _styleFlagsEx{_defaultStyleFlagsEx} {
    if(_refCount == 0) {
        if(Register()) {
            ++_refCount;
        }
    }
    switch(desc.mode) {
    case RHIOutputMode::Windowed:
        _styleFlags = _defaultWindowedStyleFlags;
        break;
    case RHIOutputMode::Borderless_Fullscreen:
        _styleFlags = _defaultBorderlessStyleFlags;
        break;
    }
    RECT r{};
    r.top = static_cast<long>(desc.position.y);
    r.left = static_cast<long>(desc.position.x);
    r.bottom = r.top + desc.dimensions.y;
    r.right = r.left + desc.dimensions.x;

    ::AdjustWindowRectEx(&r, _styleFlags, _hasMenu, _styleFlagsEx);

    _positionX = desc.position.x - r.left;
    _positionY = desc.position.y - r.top;
    _width = r.right - r.left;
    _height = r.bottom - r.top;
    _oldclientWidth = _clientWidth;
    _oldclientHeight = _clientHeight;
    _clientWidth = desc.dimensions.x;
    _clientHeight = desc.dimensions.y;
}

Window::~Window() noexcept {
    Close();
    if(_refCount != 0) {
        --_refCount;
        if(_refCount == 0) {
            GUARANTEE_OR_DIE(Unregister(), "Failed to unregister window class");
        }
    }
}

void Window::Open() noexcept {
    if(IsOpen()) {
        return;
    }
    if(Create()) {
        Show();
        SetForegroundWindow();
        SetFocus();

        HCURSOR cursor = ::LoadCursor(nullptr, IDC_ARROW);
        ::SetCursor(cursor);
    }
}

void Window::Close() noexcept {
    ::DestroyWindow(_hWnd);
}

void Window::Show() noexcept {
    ::ShowWindow(_hWnd, SW_SHOW);
}

void Window::Hide() noexcept {
    ::ShowWindow(_hWnd, SW_HIDE);
}

void Window::UnHide() noexcept {
    Show();
}

bool Window::IsOpen() const noexcept {
    return 0 != ::IsWindow(_hWnd);
}

bool Window::IsClosed() const noexcept {
    return !IsOpen();
}

bool Window::IsWindowed() const noexcept {
    return _currentDisplayMode == RHIOutputMode::Windowed;
}

bool Window::IsFullscreen() const noexcept {
    return _currentDisplayMode == RHIOutputMode::Borderless_Fullscreen;
}

IntVector2 Window::GetDimensions() const noexcept {
    return IntVector2(_width, _height);
}

IntVector2 Window::GetClientDimensions() const noexcept {
    return IntVector2(_clientWidth, _clientHeight);
}

IntVector2 Window::GetPosition() const noexcept {
    return IntVector2(_positionX, _positionY);
}

IntVector2 Window::GetDesktopResolution() noexcept {
    const auto desktop = ::GetDesktopWindow();
    RECT desktop_rect{};
    const auto error_msg = []() {
        const auto err = ::GetLastError();
        return StringUtils::FormatWindowsMessage(err);
    };
    GUARANTEE_OR_DIE(!::GetClientRect(desktop, &desktop_rect), error_msg().c_str());
    return IntVector2{desktop_rect.right - desktop_rect.left, desktop_rect.bottom - desktop_rect.top};
}

void Window::SetDimensionsAndPosition(const IntVector2& new_position, const IntVector2& new_size) noexcept {
    RECT r{};
    r.top = static_cast<long>(new_position.y);
    r.left = static_cast<long>(new_position.x);
    r.bottom = r.top + new_size.y;
    r.right = r.left + new_size.x;

    _positionX = r.left;
    _positionY = r.top;
    _width = r.right - r.left;
    _height = r.bottom - r.top;
    _oldclientWidth = _clientWidth;
    _oldclientHeight = _clientHeight;
    _clientWidth = new_size.x;
    _clientHeight = new_size.y;
}

void Window::SetPosition(const IntVector2& new_position) noexcept {
    RECT r{};
    r.top = static_cast<long>(new_position.y);
    r.left = static_cast<long>(new_position.x);
    const auto dims = GetDimensions();
    r.bottom = r.top + dims.y;
    r.right = r.left + dims.x;
    ::AdjustWindowRect(&r, _styleFlags, FALSE);
    ::SetWindowPos(_hWnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);
    _positionX = r.left;
    _positionY = r.top;
    _width = r.right - r.left;
    _height = r.bottom - r.top;
    _oldclientWidth = _clientWidth;
    _oldclientHeight = _clientHeight;
    _clientWidth = dims.x;
    _clientHeight = dims.y;
}

void Window::SetDimensions(const IntVector2& new_dimensions) noexcept {
    RECT r{};
    const auto pos = GetPosition();
    r.top = static_cast<long>(pos.y);
    r.left = static_cast<long>(pos.x);
    r.bottom = r.top + new_dimensions.y;
    r.right = r.left + new_dimensions.x;
    ::AdjustWindowRect(&r, _styleFlags, FALSE);
    _positionX = r.left;
    _positionY = r.top;
    _width = r.right - r.left;
    _height = r.bottom - r.top;
    _oldclientWidth = _clientWidth;
    _oldclientHeight = _clientHeight;
    _clientWidth = new_dimensions.x;
    _clientHeight = new_dimensions.y;
}

void Window::SetForegroundWindow() noexcept {
    ::SetForegroundWindow(_hWnd);
}

void Window::SetFocus() noexcept {
    ::SetFocus(_hWnd);
}

void* Window::GetWindowHandle() const noexcept {
    return _hWnd;
}

void Window::SetWindowHandle(void* hWnd) noexcept {
    _hWnd = static_cast<HWND>(hWnd);
}

HDC Window::GetWindowDeviceContext() const noexcept {
    return _hdc;
}

const RHIOutputMode& Window::GetDisplayMode() const noexcept {
    return _currentDisplayMode;
}

void Window::SetDisplayMode(const RHIOutputMode& display_mode) noexcept {
    if(display_mode == _currentDisplayMode) {
        return;
    }

    static WINDOWPLACEMENT g_wpPrev = {sizeof(g_wpPrev)};
    auto dwStyle = ::GetWindowLongPtr(_hWnd, GWL_STYLE);

    _currentDisplayMode = display_mode;
    switch(_currentDisplayMode) {
    case RHIOutputMode::Windowed: {
        ::SetWindowLongPtr(_hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        ::SetWindowPlacement(_hWnd, &g_wpPrev);
        ::SetWindowPos(_hWnd, nullptr, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        break;
    }
    case RHIOutputMode::Borderless_Fullscreen: {
        MONITORINFO mi = {sizeof(mi)};
        if(::GetWindowPlacement(_hWnd, &g_wpPrev) && ::GetMonitorInfo(::MonitorFromWindow(_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            ::SetWindowLongPtr(_hWnd, GWL_STYLE,
                               dwStyle & ~WS_OVERLAPPEDWINDOW);
#ifdef RENDER_DEBUG
            ::SetWindowPos(_hWnd, HWND_NOTOPMOST,
                           mi.rcMonitor.left, mi.rcMonitor.top,
                           mi.rcMonitor.right - mi.rcMonitor.left,
                           mi.rcMonitor.bottom - mi.rcMonitor.top,
                           SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
#else
            ::SetWindowPos(_hWnd, HWND_TOP,
                           mi.rcMonitor.left, mi.rcMonitor.top,
                           mi.rcMonitor.right - mi.rcMonitor.left,
                           mi.rcMonitor.bottom - mi.rcMonitor.top,
                           SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
#endif
        }
        break;
    }
    default:
        break;
    }
    Show();
}

void Window::SetTitle(const std::string& title) noexcept {
    _title = title;
    ::SetWindowTextA(_hWnd, _title.data());
}

const std::string& Window::GetTitle() const noexcept {
    return _title;
}

bool Window::Register() noexcept {
    _hInstance = GetModuleHandle(nullptr);
    memset(&_wc, 0, sizeof(_wc));
    auto window_class_name = "Simple Window Class";
    _wc.cbSize = sizeof(_wc);
    _wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    _wc.lpfnWndProc = EngineMessageHandlingProcedure;
    _wc.cbClsExtra = 0;
    _wc.cbWndExtra = 0;
    _wc.hInstance = _hInstance;
    auto path = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/icon.ico";
    HICON iconTaskbar = (HICON)::LoadImage(nullptr, path.string().c_str(), IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
    _wc.hIcon = iconTaskbar;
    _wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    _wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    _wc.lpszMenuName = NULL;
    _wc.lpszClassName = window_class_name;
    HICON iconTitlebar = (HICON)::LoadImage(nullptr, path.string().c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    _wc.hIconSm = iconTitlebar;
    return 0 != RegisterClassEx(&_wc);
}

bool Window::Unregister() noexcept {
    return 0 != ::UnregisterClass(_wc.lpszClassName, nullptr);
}

bool Window::Create() noexcept {
    _hWnd = ::CreateWindowEx(
    _styleFlagsEx,          // Optional window styles.
    _wc.lpszClassName,      // Window class
    _title.c_str(),         // Window text
    _styleFlags,            // Window style
    _positionX, _positionY, //Position XY
    _width, _height,        //Size WH
    nullptr,                // Parent window
    nullptr,                // Menu
    _hInstance,             // Instance handle
    this                    // Additional application data
    );
    _hdc = ::GetDCEx(_hWnd, nullptr, 0);

    return _hWnd != nullptr;
}
