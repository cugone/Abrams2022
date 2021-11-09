#include "Engine/Core/EngineBase.hpp"

#include "Engine/Core/App.hpp"
#include "Engine/Core/EngineCommon.hpp"

#include "Engine/RHI/RHIOutput.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IConsoleService.hpp"
#include "Engine/Services/IRendererService.hpp"

Window* GetWindowFromHwnd(HWND hwnd) {
    Window* wnd = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    return wnd;
}

//-----------------------------------------------------------------------------------------------
LRESULT CALLBACK EngineMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam) {
    //Handles application-specific window setup such as icons.
    Window* window = GetWindowFromHwnd(windowHandle);
    if(window && window->custom_message_handler) {
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
    default: {
        return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
    }
    }
}

void RunMessagePump() noexcept {
    MSG msg{};
    for(;;) {
        if(const bool hasMsg = !!::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE); !hasMsg) {
            break;
        }
        const auto& console = ServiceLocator::get<IConsoleService>();
        const auto hAccelTable = static_cast<HACCEL>(console.GetAcceleratorTable());
        const auto& renderer = ServiceLocator::get<IRendererService>();
        const auto hWnd = static_cast<HWND>(renderer.GetOutput()->GetWindow()->GetWindowHandle());
        if(!::TranslateAccelerator(hWnd, hAccelTable, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}
