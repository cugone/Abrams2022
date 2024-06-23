#pragma once

#include "Engine/Core/BuildConfig.hpp"

#if defined(PLATFORM_WINDOWS)

#include "Engine/Math/AABB2.hpp"
#include <string>

    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <windowsx.h>
    #include <KnownFolders.h>
    #include <ShObjIdl.h>
    #include <processthreadsapi.h>
    #include <shellapi.h>
    #include <wincrypt.h>
    #include <Psapi.h>
    #pragma comment(lib, "Crypt32.lib")


    #ifdef NETWORKING
        #include <WS2tcpip.h>
        #include <WinSock2.h>

        #pragma comment(lib, "ws2_32.lib")
    #endif

AABB2 RectToAABB2(const RECT& rect) noexcept;
RECT AABB2ToRect(const AABB2& aabb2) noexcept;
std::wstring GetCommandLineArgs() noexcept;

#include "Engine/Platform/Windows/WindowsWindow.hpp"

#endif
