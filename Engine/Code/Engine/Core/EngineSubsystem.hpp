#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include <cstdint>

// clang-format off
enum class WindowsSystemMessage : unsigned int {
    App_DeviceChanged
    , Clipboard_Clear
    , Clipboard_Copy
    , Clipboard_Cut
    , Clipboard_Paste
    , Clipboard_AskCbFormatName
    , Clipboard_ChangeCbChain
    , Clipboard_ClipboardUpdate
    , Clipboard_DestroyClipboard
    , ClipboardDrawClipboard
    , Clipboard_HScrollClipboard
    , Clipboard_PaintClipboard
    , Clipboard_RenderAllFormats
    , ClipboardRenderFormat
    , Clipboard_SizeClipboard
    , Clipboard_VScrollClipboard
    , Cursor_SetCursor
    , Keyboard_Activate
    , Keyboard_AppCommand
    , Keyboard_Char
    , Keyboard_DeadChar
    , Keyboard_HotKey
    , Keyboard_KeyDown
    , Keyboard_KeyUp
    , Keyboard_KillFocus
    , Keyboard_SetFocus
    , Keyboard_SysDeadChar
    , Keyboard_SysKeyDown
    , Keyboard_SysKeyUp
    , Keyboard_UniChar
    , Keyboard_Help
    , Mouse_RawInput
    , Mouse_CaptureChanged
    , Mouse_LButtonDblClk
    , Mouse_LButtonDown
    , Mouse_LButtonUp
    , Mouse_MButtonDblClk
    , Mouse_MButtonDown
    , Mouse_MButtonUp
    , Mouse_MouseActivate
    , Mouse_MouseHover
    , Mouse_MouseHWheel
    , Mouse_MouseLeave
    , Mouse_MouseMove
    , Mouse_MouseWheel
    , Mouse_NcLButtonDblClk
    , Mouse_NcLButtonDown
    , Mouse_NcLButtonUp
    , Mouse_NcMButtonDblClk
    , Mouse_NcMButtonDown
    , Mouse_NcMButtonUp
    , Mouse_NcMouseHover
    , Mouse_NcMouseLeave
    , Mouse_NcMouseMove
    , Mouse_NcRButtonDblClk
    , Mouse_NcRButtonDown
    , Mouse_NcRButtonUp
    , Mouse_NcXButtonDblClk
    , Mouse_McXButtonDown
    , Mouse_NcXButtonUp
    , Mouse_RButtonDblClk
    , Mouse_RButtonDown
    , Mouse_RButtonUp
    , Mouse_XButtonDblClk
    , Mouse_XButtonDown
    , Mouse_XButtonUp
    , Window_ActivateApp
    , Window_CancelMode
    , Window_ChildActivate
    , Window_Close
    , Window_Compacting
    , Window_Create
    , Window_Destroy
    , Window_DpiChanged
    , Window_Enable
    , Window_EnterSizeMove
    , Window_ExitSizeMove
    , Window_GetIcon
    , Window_GetMinMaxInfo
    , Window_InputLangChange
    , Window_InputLangChangeRequest
    , Window_Move
    , Window_Moving
    , Window_NcActivate
    , Window_NcCalcSize
    , Window_NcCreate
    , Window_NcDestroy
    , Window_Null
    , Window_QueryDragIcon
    , Window_QueryOpen
    , Window_Quit
    , Window_SetIcon
    , Window_ShowWindow
    , Window_Size
    , Window_Sizing
    , Window_StyleChanged
    , Window_StyleChanging
    , Window_ThemeChanged
    , Window_UserChanged
    , Window_WindowPosChanged
    , Window_WindowPosChanging
    , Menu_Command
    , Menu_SysCommand
    , Message_Not_Supported
};
// clang-format on

struct EngineMessage64 {
    WindowsSystemMessage wmMessageCode = WindowsSystemMessage::Message_Not_Supported;
    unsigned int nativeMessage = 0U;
    void* hWnd = nullptr;
    uint64_t wparam = 0ULL;
    int64_t lparam = 0ULL;
};
struct EngineMessage32 {
    WindowsSystemMessage wmMessageCode = WindowsSystemMessage::Message_Not_Supported;
    unsigned int nativeMessage = 0U;
    void* hWnd = nullptr;
    unsigned int wparam = 0U;
    long lparam = 0L;
};

#ifdef _WIN64
    #define EngineMessage EngineMessage64
#else
    #define EngineMessage EngineMessage32
#endif

// clang-format off
enum class WindowResizeType {
    Maximized
    , Minimized
    , Restored
    , MaxHide
    , MaxShow
};
//clang-format on

class EngineSubsystem {
public:
    virtual ~EngineSubsystem() noexcept = 0;

    virtual void Initialize() noexcept = 0;
    virtual void BeginFrame() noexcept = 0;
    virtual void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept = 0;
    virtual void Render() const noexcept = 0;
    virtual void EndFrame() noexcept = 0;

    [[nodiscard]] virtual bool ProcessSystemMessage([[maybe_unused]] const EngineMessage& msg) noexcept;

    [[nodiscard]] static WindowsSystemMessage GetWindowsSystemMessageFromUintMessage(unsigned int wmMessage) noexcept;
    void SetNextHandler(EngineSubsystem* next_handler) noexcept;

protected:
    [[nodiscard]] static WindowResizeType GetResizeTypeFromWmSize(const EngineMessage& msg) noexcept;

private:
    EngineSubsystem* _next_subsystem = nullptr;
};