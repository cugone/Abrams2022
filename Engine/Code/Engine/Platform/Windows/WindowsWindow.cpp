#include "Engine/Platform/Windows/WindowsWindow.hpp"

#if defined(PLATFORM_WINDOWS)

#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/RHI/RHITypes.hpp"

#include <algorithm>


[[nodiscard]] Window* GetWindowFromHwnd(HWND hwnd);
LRESULT CALLBACK EngineMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam);


Window* GetWindowFromHwnd(HWND hwnd) {
    Window* wnd = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return wnd;
}

//-----------------------------------------------------------------------------------------------
LRESULT CALLBACK EngineMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam) {
    //Handles application-specific window setup such as icons.
    if(Window* window = GetWindowFromHwnd(windowHandle); window && window->custom_message_handler) {
        const auto wasProcessed = window->custom_message_handler(windowHandle, wmMessageCode, wParam, lParam);
        if(wasProcessed) {
            return 0;
        }
    }

    switch(wmMessageCode) {
    case WM_CREATE: {
        CREATESTRUCT* cp = (CREATESTRUCT*)lParam;
        Window* wnd = (Window*)cp->lpCreateParams;
        ::SetWindowLongPtr(windowHandle, GWLP_USERDATA, (LONG_PTR)wnd);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        ::BeginPaint(windowHandle, &ps);
        ::EndPaint(windowHandle, &ps);
        return 1;
    }
    case WM_SETICON: {
        switch(wParam) {
        case ICON_BIG: {
            break;
        }
        case ICON_SMALL: {
            break;
        }
        }
        return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
    }
    case WM_GETICON: {
        switch(wParam) {
        case ICON_BIG: {
            break;
        }
        case ICON_SMALL: {
            break;
        }
        case ICON_SMALL2: {
            break;
        }
        }
        return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
    }
    default: {
        return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
    }
    }
}


std::unique_ptr<Window> Window::Create(const WindowDesc& desc) {
    return std::make_unique<WindowsWindow>(desc);
}

WindowsWindow::WindowsWindow() noexcept
: Window()
, m_styleFlags{m_defaultWindowedStyleFlags}
, m_styleFlagsEx{m_defaultStyleFlagsEx}
{
    if(m_refCount == 0) {
        if(Register()) {
            ++m_refCount;
        }
    }

    RECT desktopRect;
    HWND desktopHwnd = ::GetDesktopWindow();
    ::GetClientRect(desktopHwnd, &desktopRect);
    ::AdjustWindowRectEx(&desktopRect, m_styleFlags, m_hasMenu, m_styleFlagsEx);
}

WindowsWindow::WindowsWindow(const IntVector2& position, const IntVector2& dimensions) noexcept
: Window()
, m_styleFlags{m_defaultWindowedStyleFlags}
, m_styleFlagsEx{m_defaultStyleFlagsEx}
{
    if(m_refCount == 0) {
        if(Register()) {
            ++m_refCount;
        }
    }
    RECT r{};
    r.top = static_cast<long>(position.y);
    r.left = static_cast<long>(position.x);
    r.bottom = r.top + dimensions.y;
    r.right = r.left + dimensions.x;

    ::AdjustWindowRectEx(&r, m_styleFlags, m_hasMenu, m_styleFlagsEx);

    m_positionX = position.x - r.left;
    m_positionY = position.y - r.top;
    m_width = r.right - r.left;
    m_height = r.bottom - r.top;
    m_oldclientWidth = m_clientWidth;
    m_oldclientHeight = m_clientHeight;
    m_clientWidth = dimensions.x;
    m_clientHeight = dimensions.y;
}

WindowsWindow::WindowsWindow(const WindowDesc& desc) noexcept
: Window()
, m_styleFlags{m_defaultWindowedStyleFlags}
, m_styleFlagsEx{m_defaultStyleFlagsEx}
{
    if(m_refCount == 0) {
        if(Register()) {
            ++m_refCount;
        }
    }
    switch(desc.mode) {
    case RHIOutputMode::Windowed:
        m_styleFlags = m_defaultWindowedStyleFlags;
        break;
    case RHIOutputMode::Borderless_Fullscreen:
        m_styleFlags = m_defaultBorderlessStyleFlags;
        break;
    }
    RECT r{};
    r.top = static_cast<long>(desc.position.y);
    r.left = static_cast<long>(desc.position.x);
    r.bottom = r.top + desc.dimensions.y;
    r.right = r.left + desc.dimensions.x;

    ::AdjustWindowRectEx(&r, m_styleFlags, m_hasMenu, m_styleFlagsEx);

    m_positionX = desc.position.x - r.left;
    m_positionY = desc.position.y - r.top;
    m_width = r.right - r.left;
    m_height = r.bottom - r.top;
    m_oldclientWidth = m_clientWidth;
    m_oldclientHeight = m_clientHeight;
    m_clientWidth = desc.dimensions.x;
    m_clientHeight = desc.dimensions.y;
}

WindowsWindow::~WindowsWindow() noexcept {
    Close();
    if(m_refCount != 0) {
        --m_refCount;
        if(m_refCount == 0) {
            GUARANTEE_OR_DIE(Unregister(), "Failed to unregister window class");
        }
    }
}

void WindowsWindow::Open() noexcept {
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

void WindowsWindow::Close() noexcept {
    ::DestroyWindow(m_hWnd);
}

void WindowsWindow::Show() noexcept {
    ::ShowWindow(m_hWnd, SW_SHOW);
}

void WindowsWindow::Hide() noexcept {
    ::ShowWindow(m_hWnd, SW_HIDE);
}

void WindowsWindow::UnHide() noexcept {
    Show();
}

bool WindowsWindow::IsOpen() const noexcept {
    return 0 != ::IsWindow(m_hWnd);
}

bool WindowsWindow::IsClosed() const noexcept {
    return !IsOpen();
}

bool WindowsWindow::IsWindowed() const noexcept {
    return m_currentDisplayMode == RHIOutputMode::Windowed;
}

bool WindowsWindow::IsFullscreen() const noexcept {
    return m_currentDisplayMode == RHIOutputMode::Borderless_Fullscreen;
}

IntVector2 WindowsWindow::GetDimensions() const noexcept {
    return IntVector2(m_width, m_height);
}

IntVector2 WindowsWindow::GetClientDimensions() const noexcept {
    return IntVector2(m_clientWidth, m_clientHeight);
}

IntVector2 WindowsWindow::GetPosition() const noexcept {
    return IntVector2(m_positionX, m_positionY);
}

void WindowsWindow::SetDimensionsAndPosition(const IntVector2& new_position, const IntVector2& new_size) noexcept {
    RECT r{};
    r.top = static_cast<long>(new_position.y);
    r.left = static_cast<long>(new_position.x);
    r.bottom = r.top + new_size.y;
    r.right = r.left + new_size.x;

    m_positionX = r.left;
    m_positionY = r.top;
    m_width = r.right - r.left;
    m_height = r.bottom - r.top;
    m_oldclientWidth = m_clientWidth;
    m_oldclientHeight = m_clientHeight;
    m_clientWidth = new_size.x;
    m_clientHeight = new_size.y;
}

void WindowsWindow::SetPosition(const IntVector2& new_position) noexcept {
    RECT r{};
    r.top = static_cast<long>(new_position.y);
    r.left = static_cast<long>(new_position.x);
    const auto dims = GetDimensions();
    r.bottom = r.top + dims.y;
    r.right = r.left + dims.x;
    ::AdjustWindowRect(&r, m_styleFlags, FALSE);
    ::SetWindowPos(m_hWnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);
    m_positionX = r.left;
    m_positionY = r.top;
    m_width = r.right - r.left;
    m_height = r.bottom - r.top;
    m_oldclientWidth = m_clientWidth;
    m_oldclientHeight = m_clientHeight;
    m_clientWidth = dims.x;
    m_clientHeight = dims.y;
}

void WindowsWindow::SetDimensions(const IntVector2& new_dimensions) noexcept {
    RECT r{};
    const auto pos = GetPosition();
    r.top = static_cast<long>(pos.y);
    r.left = static_cast<long>(pos.x);
    r.bottom = r.top + new_dimensions.y;
    r.right = r.left + new_dimensions.x;
    ::AdjustWindowRect(&r, m_styleFlags, FALSE);
    m_positionX = r.left;
    m_positionY = r.top;
    m_width = r.right - r.left;
    m_height = r.bottom - r.top;
    m_oldclientWidth = m_clientWidth;
    m_oldclientHeight = m_clientHeight;
    m_clientWidth = new_dimensions.x;
    m_clientHeight = new_dimensions.y;
}

void WindowsWindow::SetForegroundWindow() noexcept {
    ::SetForegroundWindow(m_hWnd);
}

void WindowsWindow::SetFocus() noexcept {
    ::SetFocus(m_hWnd);
}

void* WindowsWindow::GetWindowHandle() const noexcept {
    return m_hWnd;
}

void WindowsWindow::SetWindowHandle(void* hWnd) noexcept {
    m_hWnd = static_cast<HWND>(hWnd);
}

void* WindowsWindow::GetWindowDeviceContext() const noexcept {
    return m_hdc;
}

const RHIOutputMode& WindowsWindow::GetDisplayMode() const noexcept {
    return m_currentDisplayMode;
}

void WindowsWindow::SetDisplayMode(const RHIOutputMode& display_mode) noexcept {
    if(display_mode == m_currentDisplayMode) {
        return;
    }

    static WINDOWPLACEMENT g_wpPrev = {sizeof(g_wpPrev)};
    auto dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);

    m_currentDisplayMode = display_mode;
    switch(m_currentDisplayMode) {
        case RHIOutputMode::Windowed:
        {
            ::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
            ::SetWindowPlacement(m_hWnd, &g_wpPrev);
            ::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            break;
        }
        case RHIOutputMode::Borderless_Fullscreen:
        {
        #ifdef IN_EDITOR
            ::SetWindowLongPtr(m_hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
            ::SetWindowPlacement(m_hWnd, &g_wpPrev);
            ::SetWindowPos(m_hWnd, nullptr, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        #else
            MONITORINFO mi = {sizeof(mi)};
            if(::GetWindowPlacement(m_hWnd, &g_wpPrev) && ::GetMonitorInfo(::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi)) {
                ::SetWindowLongPtr(m_hWnd, GWL_STYLE,
                                   (dwStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            #ifdef RENDER_DEBUG
                ::SetWindowPos(m_hWnd, HWND_TOPMOST,
                               mi.rcMonitor.left, mi.rcMonitor.top,
                               mi.rcMonitor.right - mi.rcMonitor.left,
                               mi.rcMonitor.bottom - mi.rcMonitor.top,
                               SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            #else
                ::SetWindowPos(m_hWnd, HWND_TOPMOST,
                               mi.rcMonitor.left, mi.rcMonitor.top,
                               mi.rcMonitor.right - mi.rcMonitor.left,
                               mi.rcMonitor.bottom - mi.rcMonitor.top,
                               SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            #endif
        #endif
            }
            break;
        }
    default:
        break;
    }
    Show();
    SetForegroundWindow();
    SetFocus();
    }

void WindowsWindow::SetTitle(const std::string& title) noexcept {
    m_title = title;
    ::SetWindowTextA(m_hWnd, m_title.data());
}

const std::string& WindowsWindow::GetTitle() const noexcept {
    return m_title;
}

bool WindowsWindow::Register() noexcept {
    m_hInstance = GetModuleHandle(nullptr);
    memset(&m_wc, 0, sizeof(m_wc));
    auto window_class_name = "Simple Window Class";
    m_wc.cbSize = sizeof(m_wc);
    m_wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    m_wc.lpfnWndProc = EngineMessageHandlingProcedure;
    m_wc.cbClsExtra = 0;
    m_wc.cbWndExtra = 0;
    m_wc.hInstance = m_hInstance;
    auto path = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / "Resources/Icons/icon.ico";
    HICON iconTaskbar = (HICON)::LoadImage(nullptr, path.string().c_str(), IMAGE_ICON, 64, 64, LR_LOADFROMFILE);
    m_wc.hIcon = iconTaskbar;
    m_wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    m_wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    m_wc.lpszMenuName = NULL;
    m_wc.lpszClassName = window_class_name;
    HICON iconTitlebar = (HICON)::LoadImage(nullptr, path.string().c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    m_wc.hIconSm = iconTitlebar;
    return 0 != RegisterClassEx(&m_wc);
}

bool WindowsWindow::Unregister() noexcept {
    return 0 != ::UnregisterClass(m_wc.lpszClassName, nullptr);
}

bool WindowsWindow::Create() noexcept {
    m_hWnd = ::CreateWindowEx(
    m_styleFlagsEx,          // Optional window styles.
    m_wc.lpszClassName,      // Window class
    m_title.c_str(),         // Window text
    m_styleFlags,            // Window style
    m_positionX, m_positionY, //Position XY
    m_width, m_height,        //Size WH
    nullptr,                // Parent window
    nullptr,                // Menu
    m_hInstance,             // Instance handle
    this                    // Additional application data
    );
    m_hdc = ::GetDCEx(m_hWnd, nullptr, 0);

    return m_hWnd != nullptr;
}

#endif
