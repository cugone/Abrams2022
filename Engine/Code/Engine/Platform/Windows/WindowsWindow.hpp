#pragma once

#include "Engine/Core/BuildConfig.hpp"

#if defined(PLATFORM_WINDOWS)

#include "Engine/Platform/Win.hpp"

#include "Engine/Math/IntVector2.hpp"
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
    RHIOutputMode m_currentDisplayMode = RHIOutputMode::Windowed;
    HWND m_hWnd{};
    HDC m_hdc{};
    HINSTANCE m_hInstance{};
    std::string m_title{"Created with Abrams 2022 (c) Casey Ugone "};
    INT m_cmdShow{};
    WNDCLASSEX m_wc{};
    int m_positionX{};
    int m_positionY{};
    unsigned int m_width{1600u};
    unsigned int m_height{900u};
    unsigned int m_oldclientWidth{1600u};
    unsigned int m_oldclientHeight{900u};
    unsigned int m_clientWidth{1600u};
    unsigned int m_clientHeight{900u};
    unsigned long m_styleFlags{};
    unsigned long m_styleFlagsEx{};
    bool m_hasMenu{};
    static inline std::size_t m_refCount{0u};
    static inline constexpr unsigned long m_defaultWindowedStyleFlags{WS_OVERLAPPEDWINDOW | WS_BORDER /* WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX */};
    static inline constexpr unsigned long m_defaultBorderlessStyleFlags{WS_POPUP};
    static inline constexpr unsigned long m_defaultStyleFlagsEx{WS_EX_APPWINDOW | WS_EX_ACCEPTFILES};
};

#endif
