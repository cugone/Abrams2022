#include "Engine/Core/BuildConfig.hpp"

#if defined(PLATFORM_WINDOWS)

#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"

#include "Editor/Editor.hpp"

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 28251)
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);
    const auto cmdString = StringUtils::ConvertUnicodeToMultiByte(std::wstring(pCmdLine ? pCmdLine : L""));
    Engine<Editor>::Initialize("Abrams Game Engine 2022", cmdString);
    Engine<Editor>::Run();
    Engine<Editor>::Shutdown();
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif