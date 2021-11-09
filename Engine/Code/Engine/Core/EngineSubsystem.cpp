#include "Engine/Core/EngineSubsystem.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Platform/Win.hpp"

EngineSubsystem::~EngineSubsystem() noexcept {
    _next_subsystem = nullptr;
}

bool EngineSubsystem::ProcessSystemMessage(const EngineMessage& msg) noexcept {
    if(ProcessSystemMessage(msg)) {
        return true;
    }
    if(_next_subsystem) {
        return _next_subsystem->EngineSubsystem::ProcessSystemMessage(msg);
    }
    return false;
}

WindowResizeType EngineSubsystem::GetResizeTypeFromWmSize(const EngineMessage& msg) noexcept {
    GUARANTEE_OR_DIE(msg.wmMessageCode == WindowsSystemMessage::Window_Size, "Message passed to GetResizeTypeFromWmSize is not an appropriate message type.");
    switch(msg.wparam) {
    case SIZE_RESTORED: return WindowResizeType::Restored;
    case SIZE_MINIMIZED: return WindowResizeType::Minimized;
    case SIZE_MAXIMIZED: return WindowResizeType::Maximized;
    case SIZE_MAXSHOW: return WindowResizeType::MaxShow;
    case SIZE_MAXHIDE: return WindowResizeType::MaxHide;
    default: ERROR_AND_DIE("Number of WM_SIZE WPARAM values have changed.");
    }
}

WindowsSystemMessage EngineSubsystem::GetWindowsSystemMessageFromUintMessage(unsigned int wmMessage) noexcept {
    switch(wmMessage) {
    case WM_DEVICECHANGE: return WindowsSystemMessage::App_DeviceChanged;
    case WM_CLEAR: return WindowsSystemMessage::Clipboard_Clear;
    case WM_COPY: return WindowsSystemMessage::Clipboard_Copy;
    case WM_CUT: return WindowsSystemMessage::Clipboard_Cut;
    case WM_PASTE: return WindowsSystemMessage::Clipboard_Paste;
    case WM_ASKCBFORMATNAME: return WindowsSystemMessage::Clipboard_AskCbFormatName;
    case WM_CHANGECBCHAIN: return WindowsSystemMessage::Clipboard_ChangeCbChain;
    case WM_CLIPBOARDUPDATE: return WindowsSystemMessage::Clipboard_ClipboardUpdate;
    case WM_DESTROYCLIPBOARD: return WindowsSystemMessage::Clipboard_DestroyClipboard;
    case WM_DRAWCLIPBOARD: return WindowsSystemMessage::ClipboardDrawClipboard;
    case WM_HSCROLLCLIPBOARD: return WindowsSystemMessage::Clipboard_HScrollClipboard;
    case WM_PAINTCLIPBOARD: return WindowsSystemMessage::Clipboard_PaintClipboard;
    case WM_RENDERALLFORMATS: return WindowsSystemMessage::Clipboard_RenderAllFormats;
    case WM_RENDERFORMAT: return WindowsSystemMessage::ClipboardRenderFormat;
    case WM_SIZECLIPBOARD: return WindowsSystemMessage::Clipboard_SizeClipboard;
    case WM_VSCROLLCLIPBOARD: return WindowsSystemMessage::Clipboard_VScrollClipboard;
    case WM_SETCURSOR: return WindowsSystemMessage::Cursor_SetCursor;
    case WM_ACTIVATE: return WindowsSystemMessage::Keyboard_Activate;
    case WM_APPCOMMAND: return WindowsSystemMessage::Keyboard_AppCommand;
    case WM_CHAR: return WindowsSystemMessage::Keyboard_Char;
    case WM_DEADCHAR: return WindowsSystemMessage::Keyboard_DeadChar;
    case WM_HOTKEY: return WindowsSystemMessage::Keyboard_HotKey;
    case WM_KEYDOWN: return WindowsSystemMessage::Keyboard_KeyDown;
    case WM_KEYUP: return WindowsSystemMessage::Keyboard_KeyUp;
    case WM_KILLFOCUS: return WindowsSystemMessage::Keyboard_KillFocus;
    case WM_SETFOCUS: return WindowsSystemMessage::Keyboard_SetFocus;
    case WM_SYSDEADCHAR: return WindowsSystemMessage::Keyboard_SysDeadChar;
    case WM_SYSKEYDOWN: return WindowsSystemMessage::Keyboard_SysKeyDown;
    case WM_SYSKEYUP: return WindowsSystemMessage::Keyboard_SysKeyUp;
    case WM_UNICHAR: return WindowsSystemMessage::Keyboard_UniChar;
    case WM_HELP: return WindowsSystemMessage::Keyboard_Help;
    case WM_INPUT: return WindowsSystemMessage::Mouse_RawInput;
    case WM_CAPTURECHANGED: return WindowsSystemMessage::Mouse_CaptureChanged;
    case WM_LBUTTONDBLCLK: return WindowsSystemMessage::Mouse_LButtonDblClk;
    case WM_LBUTTONDOWN: return WindowsSystemMessage::Mouse_LButtonDown;
    case WM_LBUTTONUP: return WindowsSystemMessage::Mouse_LButtonUp;
    case WM_MBUTTONDBLCLK: return WindowsSystemMessage::Mouse_MButtonDblClk;
    case WM_MBUTTONDOWN: return WindowsSystemMessage::Mouse_MButtonDown;
    case WM_MBUTTONUP: return WindowsSystemMessage::Mouse_MButtonUp;
    case WM_MOUSEACTIVATE: return WindowsSystemMessage::Mouse_MouseActivate;
    case WM_MOUSEHOVER: return WindowsSystemMessage::Mouse_MouseHover;
    case WM_MOUSEHWHEEL: return WindowsSystemMessage::Mouse_MouseHWheel;
    case WM_MOUSELEAVE: return WindowsSystemMessage::Mouse_MouseLeave;
    case WM_MOUSEMOVE: return WindowsSystemMessage::Mouse_MouseMove;
    case WM_MOUSEWHEEL: return WindowsSystemMessage::Mouse_MouseWheel;
    case WM_NCLBUTTONDBLCLK: return WindowsSystemMessage::Mouse_NcLButtonDblClk;
    case WM_NCLBUTTONDOWN: return WindowsSystemMessage::Mouse_NcLButtonDown;
    case WM_NCLBUTTONUP: return WindowsSystemMessage::Mouse_NcLButtonUp;
    case WM_NCMBUTTONDBLCLK: return WindowsSystemMessage::Mouse_NcMButtonDblClk;
    case WM_NCMBUTTONDOWN: return WindowsSystemMessage::Mouse_NcMButtonDown;
    case WM_NCMBUTTONUP: return WindowsSystemMessage::Mouse_NcMButtonUp;
    case WM_NCMOUSEHOVER: return WindowsSystemMessage::Mouse_NcMouseHover;
    case WM_NCMOUSELEAVE: return WindowsSystemMessage::Mouse_NcMouseLeave;
    case WM_NCMOUSEMOVE: return WindowsSystemMessage::Mouse_NcMouseMove;
    case WM_NCRBUTTONDBLCLK: return WindowsSystemMessage::Mouse_NcRButtonDblClk;
    case WM_NCRBUTTONDOWN: return WindowsSystemMessage::Mouse_NcRButtonDown;
    case WM_NCRBUTTONUP: return WindowsSystemMessage::Mouse_NcRButtonUp;
    case WM_NCXBUTTONDBLCLK: return WindowsSystemMessage::Mouse_NcXButtonDblClk;
    case WM_NCXBUTTONDOWN: return WindowsSystemMessage::Mouse_McXButtonDown;
    case WM_NCXBUTTONUP: return WindowsSystemMessage::Mouse_NcXButtonUp;
    case WM_RBUTTONDBLCLK: return WindowsSystemMessage::Mouse_RButtonDblClk;
    case WM_RBUTTONDOWN: return WindowsSystemMessage::Mouse_RButtonDown;
    case WM_RBUTTONUP: return WindowsSystemMessage::Mouse_RButtonUp;
    case WM_XBUTTONDBLCLK: return WindowsSystemMessage::Mouse_XButtonDblClk;
    case WM_XBUTTONDOWN: return WindowsSystemMessage::Mouse_XButtonDown;
    case WM_XBUTTONUP: return WindowsSystemMessage::Mouse_XButtonUp;
    case WM_ACTIVATEAPP: return WindowsSystemMessage::Window_ActivateApp;
    case WM_CANCELMODE: return WindowsSystemMessage::Window_CancelMode;
    case WM_CHILDACTIVATE: return WindowsSystemMessage::Window_ChildActivate;
    case WM_CLOSE: return WindowsSystemMessage::Window_Close;
    case WM_COMPACTING: return WindowsSystemMessage::Window_Compacting;
    case WM_CREATE: return WindowsSystemMessage::Window_Create;
    case WM_DESTROY: return WindowsSystemMessage::Window_Destroy;
    case WM_DPICHANGED: return WindowsSystemMessage::Window_DpiChanged;
    case WM_ENABLE: return WindowsSystemMessage::Window_Enable;
    case WM_ENTERSIZEMOVE: return WindowsSystemMessage::Window_EnterSizeMove;
    case WM_EXITSIZEMOVE: return WindowsSystemMessage::Window_ExitSizeMove;
    case WM_GETICON: return WindowsSystemMessage::Window_GetIcon;
    case WM_GETMINMAXINFO: return WindowsSystemMessage::Window_GetMinMaxInfo;
    case WM_INPUTLANGCHANGE: return WindowsSystemMessage::Window_InputLangChange;
    case WM_INPUTLANGCHANGEREQUEST: return WindowsSystemMessage::Window_InputLangChangeRequest;
    case WM_MOVE: return WindowsSystemMessage::Window_Move;
    case WM_MOVING: return WindowsSystemMessage::Window_Moving;
    case WM_NCACTIVATE: return WindowsSystemMessage::Window_NcActivate;
    case WM_NCCALCSIZE: return WindowsSystemMessage::Window_NcCalcSize;
    case WM_NCCREATE: return WindowsSystemMessage::Window_NcCreate;
    case WM_NCDESTROY: return WindowsSystemMessage::Window_NcDestroy;
    case WM_NULL: return WindowsSystemMessage::Window_Null;
    case WM_QUERYDRAGICON: return WindowsSystemMessage::Window_QueryDragIcon;
    case WM_QUERYOPEN: return WindowsSystemMessage::Window_QueryOpen;
    case WM_QUIT: return WindowsSystemMessage::Window_Quit;
    case WM_SETICON: return WindowsSystemMessage::Window_SetIcon;
    case WM_SHOWWINDOW: return WindowsSystemMessage::Window_ShowWindow;
    case WM_SIZE: return WindowsSystemMessage::Window_Size;
    case WM_SIZING: return WindowsSystemMessage::Window_Sizing;
    case WM_STYLECHANGED: return WindowsSystemMessage::Window_StyleChanged;
    case WM_STYLECHANGING: return WindowsSystemMessage::Window_StyleChanging;
    case WM_THEMECHANGED: return WindowsSystemMessage::Window_ThemeChanged;
    case WM_USERCHANGED: return WindowsSystemMessage::Window_UserChanged;
    case WM_WINDOWPOSCHANGED: return WindowsSystemMessage::Window_WindowPosChanged;
    case WM_WINDOWPOSCHANGING: return WindowsSystemMessage::Window_WindowPosChanging;
    case WM_COMMAND: return WindowsSystemMessage::Menu_Command;
    case WM_SYSCOMMAND: return WindowsSystemMessage::Menu_SysCommand;
    default:
        return WindowsSystemMessage::Message_Not_Supported;
    }
}

void EngineSubsystem::SetNextHandler(EngineSubsystem* next_handler) noexcept {
    _next_subsystem = next_handler;
}
