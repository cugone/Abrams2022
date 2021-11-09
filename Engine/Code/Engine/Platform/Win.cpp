#include "Engine/Platform/Win.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Platform/PlatformUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

#if defined(PLATFORM_WINDOWS)

#include <commdlg.h>

AABB2 RectToAABB2(const RECT& rect) noexcept {
    return AABB2(static_cast<float>(rect.left),
                 static_cast<float>(rect.top),
                 static_cast<float>(rect.right),
                 static_cast<float>(rect.bottom));
}

RECT AABB2ToRect(const AABB2& aabb2) noexcept {
    return RECT{static_cast<long>(aabb2.mins.x),
                static_cast<long>(aabb2.mins.y),
                static_cast<long>(aabb2.maxs.x),
                static_cast<long>(aabb2.maxs.y)};
}

std::string FileDialogs::OpenFile(const char* filter = "All Files (*.*)\0*.*\0\0") noexcept {
    OPENFILENAMEA ofn{};
    CHAR szFile[MAX_PATH] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);

    const auto& renderer = ServiceLocator::get<IRendererService>();
    ofn.hwndOwner = reinterpret_cast<HWND>(renderer.GetOutput()->GetWindow()->GetWindowHandle());

    static const auto initial_dir = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData);
    static const auto initial_dir_str = initial_dir.string();
    ofn.lpstrInitialDir = initial_dir_str.c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if(::GetOpenFileNameA(&ofn)) {
        return ofn.lpstrFile;
    }
    return {};
}

std::string FileDialogs::SaveFile(const char* filter = "All Files (*.*)\0*.*\0\0") noexcept {
    OPENFILENAMEA ofn{};
    CHAR szFile[MAX_PATH] = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);

    const auto& renderer = ServiceLocator::get<IRendererService>();
    ofn.hwndOwner = reinterpret_cast<HWND>(renderer.GetOutput()->GetWindow()->GetWindowHandle());

    static const auto initial_dir = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData);
    static const auto initial_dir_str = initial_dir.string();
    ofn.lpstrInitialDir = initial_dir_str.c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if(::GetSaveFileNameA(&ofn)) {
        return ofn.lpstrFile;
    }
    return {};
}

#endif
