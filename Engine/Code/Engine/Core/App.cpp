#include "Engine/Core/App.hpp"

namespace detail {

    EngineMessage GetEngineMessageFromWindowsParams(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        EngineMessage msg{};
        msg.hWnd = hwnd;
        msg.nativeMessage = uMsg;
        msg.wmMessageCode = EngineSubsystem::GetWindowsSystemMessageFromUintMessage(uMsg);
        msg.wparam = wParam;
        msg.lparam = lParam;
        return msg;
    }

    bool CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return (g_theSubsystemHead && g_theSubsystemHead->EngineSubsystem::ProcessSystemMessage(detail::GetEngineMessageFromWindowsParams(hwnd, uMsg, wParam, lParam)));
    }

}
