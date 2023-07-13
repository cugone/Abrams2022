#include "Engine/Platform/Win.hpp"

#include "Engine/Core/StringUtils.hpp"

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

    const auto* const renderer = ServiceLocator::const_get<IRendererService>();
    ofn.hwndOwner = reinterpret_cast<HWND>(renderer->GetOutput()->GetWindow()->GetWindowHandle());

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

    const auto* const renderer = ServiceLocator::const_get<IRendererService>();
    ofn.hwndOwner = reinterpret_cast<HWND>(renderer->GetOutput()->GetWindow()->GetWindowHandle());

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

[[nodiscard]] IntVector2 Window::GetDesktopResolution() noexcept {
    const auto desktop = ::GetDesktopWindow();
    RECT desktop_rect{};
    const auto error_msg = []() {
        const auto err = ::GetLastError();
        return StringUtils::FormatWindowsMessage(err);
    };
    GUARANTEE_OR_DIE(!::GetClientRect(desktop, &desktop_rect), error_msg().c_str());
    return IntVector2{desktop_rect.right - desktop_rect.left, desktop_rect.bottom - desktop_rect.top};
}

std::wstring GetCommandLineArgs() noexcept {
    int argc = 0;
    std::vector<std::wstring> cmdLineArray;
    const auto pCmdLine = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if(argc < 2) {
        return {};
    }
    cmdLineArray.resize(argc);
    for(std::size_t i = 0u; i < argc; ++i) {
        cmdLineArray[i] = std::wstring(pCmdLine[i] ? pCmdLine[i] : L"");
        if(StringUtils::Contains(cmdLineArray[i], L" \n\t\v\f")) {
            cmdLineArray[i] = std::wstring{'\"'} + cmdLineArray[i] + std::wstring{'\"'};
        }
    }
    std::wstring result;
    const auto s = [&]() {
        std::size_t new_size{0};
        for(const auto& e : cmdLineArray) {
            new_size += e.size();
        }
        return new_size;
    }();
    result.reserve(s + std::size_t(argc - 1));
    for(const auto& e : cmdLineArray) {
        result += e + L' ';
    }
    return result;
}

std::filesystem::path FileUtils::GetExePath() noexcept {
    namespace FS = std::filesystem;
    FS::path result{};
    {
        std::basic_string<TCHAR> filename(MAX_PATH, '\0');
        while(DWORD buffer_length = ::GetModuleFileName(nullptr, filename.data(), static_cast<DWORD>(filename.size()))) {
            if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                filename.resize(filename.size() * 2);
                continue;
            }
            filename = filename.substr(0, buffer_length);
            result = FS::path(filename);
            {
                std::error_code ec{};
                if(result = FS::canonical(result, ec); !ec) {
                    result.make_preferred();
                }
            }
            return result;
        }
    }
    return result;
}


#endif
