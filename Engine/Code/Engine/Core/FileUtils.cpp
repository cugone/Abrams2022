#include "Engine/Core/FileUtils.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"

#ifdef PLATFORM_WINDOWS
#include <KnownFolders.h>
#include <Shobjidl.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <sstream>

namespace FileUtils {

GUID GetKnownPathIdForOS(const KnownPathID& pathid) noexcept;

bool WriteBufferToFile(const std::any& buffer, std::size_t size, std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    filepath = FS::absolute(filepath);
    filepath.make_preferred();
    const auto not_valid_path = FS::is_directory(filepath);
    const auto invalid = not_valid_path;
    if(invalid) {
        return false;
    }

    if(std::ofstream ofs{filepath, std::ios_base::binary}; ofs.write(reinterpret_cast<const char*>(&buffer), size)) {
        return true;
    }
    return false;
}

bool WriteBufferToFile(const std::string& buffer, std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    filepath = FS::absolute(filepath);
    filepath.make_preferred();
    const auto not_valid_path = FS::is_directory(filepath);
    const auto invalid = not_valid_path;
    if(invalid) {
        return false;
    }

    if(std::ofstream ofs{filepath}; ofs.write(reinterpret_cast<const char*>(buffer.data()), buffer.size())) {
        return true;
    }
    return false;
}

std::optional<std::vector<uint8_t>> ReadBinaryBufferFromFile(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    const auto path_not_exist = !FS::exists(filepath);
    if(path_not_exist) {
        return {};
    }
    {
        std::error_code ec{};
        if(filepath = FS::canonical(filepath, ec); ec) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return {};
        }
    }
    filepath.make_preferred();
    const auto path_is_directory = FS::is_directory(filepath);
    const auto not_valid_path = path_is_directory || path_not_exist;
    if(not_valid_path) {
        return {};
    }

    const auto byte_size = FS::file_size(filepath);
    std::vector<uint8_t> out_buffer{};
    out_buffer.resize(byte_size);
    if(std::ifstream ifs{filepath, std::ios_base::binary}; ifs.read(reinterpret_cast<char*>(out_buffer.data()), out_buffer.size())) {
        return out_buffer;
    }
    return {};
}

std::optional<std::string> ReadStringBufferFromFile(std::filesystem::path filepath) noexcept {
    namespace FS = std::filesystem;
    const auto initial_path_not_exist = !FS::exists(filepath);
    if(initial_path_not_exist) {
        return {};
    }
    {
        std::error_code ec{};
        if(filepath = FS::canonical(filepath, ec); ec) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return {};
        }
    }
    filepath.make_preferred();
    const auto canonical_path_not_exist = !FS::exists(filepath);
    const auto path_is_directory = FS::is_directory(filepath);
    const auto not_valid_path = path_is_directory || canonical_path_not_exist;
    if(not_valid_path) {
        return {};
    }

    if(std::ifstream ifs{filepath}; ifs) {
        return std::string(static_cast<const std::stringstream&>(std::stringstream() << ifs.rdbuf()).str());
    }
    return {};
}

//This version of ReadSome is intended for one-time-only reads of a portion of a TEXT file.
//If you want multiple reads use the ifstream version.
[[nodiscard]] std::optional<std::string> ReadSomeStringBufferFromFile(std::filesystem::path filepath, std::size_t pos, std::size_t count /*= 0u*/) noexcept {
    namespace FS = std::filesystem;
    const auto initial_path_not_exist = !FS::exists(filepath);
    if(initial_path_not_exist) {
        return {};
    }
    {
        std::error_code ec{};
        if(filepath = FS::canonical(filepath, ec); ec) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return {};
        }
    }
    filepath.make_preferred();
    const auto canonical_path_not_exist = !FS::exists(filepath);
    const auto path_is_directory = FS::is_directory(filepath);
    const auto not_valid_path = path_is_directory || canonical_path_not_exist;
    if(not_valid_path) {
        return {};
    }
    std::ifstream ifs{filepath};
    return ReadSomeStringBufferFromFile(ifs, pos, count);
}

//This version of ReadSome is intended for continuous or multiple reads of a portion of a TEXT file.
//If you want a one-time-only single read of a portion of a TEXT file use the filepath version.
[[nodiscard]] std::optional<std::string> ReadSomeStringBufferFromFile(std::ifstream& ifs, std::streampos pos, std::streamsize count /*= 0u*/) noexcept {
    if(!(ifs && ifs.is_open())) {
        return {};
    }
    ifs.seekg(pos, std::ios::beg); // MSVC ifstream::seekg doesn't set the fail bit, so can't early-out until the get call.
    char ch{};
    std::string result{};
    result.reserve(count);
    bool readsome{false}; //If nothing read, make std::optional::has_value false.
    while(ifs && ifs.get(ch) && count > 0) {
        result.append(1, ch);
        --count;
        readsome |= true;
    }
    return readsome ? std::make_optional(result) : std::nullopt;
}

//This version of ReadSome is intended for one-time-only reads of a portion of a BINARY file.
//If you want multiple reads use the ifstream version.
//If you do, make sure it is set to binary mode.
[[nodiscard]] std::optional<std::string> ReadSomeBinaryBufferFromFile(std::filesystem::path filepath, std::size_t pos, std::size_t count /*= 0u*/) noexcept {
    namespace FS = std::filesystem;
    const auto initial_path_not_exist = !FS::exists(filepath);
    if(initial_path_not_exist) {
        return {};
    }
    {
        std::error_code ec{};
        if(filepath = FS::canonical(filepath, ec); ec) {
            auto& logger = ServiceLocator::get<IFileLoggerService>();
            logger.LogErrorLine("File: " + filepath.string() + " is inaccessible.");
            return {};
        }
    }
    filepath.make_preferred();
    const auto canonical_path_not_exist = !FS::exists(filepath);
    const auto path_is_directory = FS::is_directory(filepath);
    const auto not_valid_path = path_is_directory || canonical_path_not_exist;
    if(not_valid_path) {
        return {};
    }
    std::ifstream ifs(filepath, std::ios_base::binary);
    return ReadSomeBinaryBufferFromFile(ifs, pos, count);
}

//This version of ReadSome is intended for continuous or multiple reads of a portion of a BINARY file.
//Make sure the file stream is set to binary mode.
//If you want a one-time-only single read of a portion of a BINARY file use the filepath version.
[[nodiscard]] std::optional<std::string> ReadSomeBinaryBufferFromFile(std::ifstream& ifs, std::streampos pos, std::streamsize count /*= 0u*/) noexcept {
    if(!(ifs && ifs.is_open())) {
        return {};
    }
    ifs.seekg(pos);
    auto result = std::string(count, '\0');
    ifs.read(result.data(), count);
    if(ifs.gcount()) {
        return result;
    }
    return {};
}

bool CreateFolders(const std::filesystem::path& filepath) noexcept {
    namespace FS = std::filesystem;
    std::filesystem::path p = filepath;
    p.make_preferred();
    std::error_code ec{};
    return FS::create_directories(p, ec);
}

bool IsContentPathId(const KnownPathID& pathid) noexcept {
    if(!IsSystemPathId(pathid)) {
        switch(pathid) {
        case KnownPathID::GameConfig: return true;
        case KnownPathID::GameData: return true;
        case KnownPathID::GameFonts: return true;
        case KnownPathID::GameMaterials: return true;
        case KnownPathID::GameLogs: return true;
        case KnownPathID::EngineConfig: return true;
        case KnownPathID::EngineData: return true;
        case KnownPathID::EngineFonts: return true;
        case KnownPathID::EngineMaterials: return true;
        case KnownPathID::EngineLogs: return true;
        case KnownPathID::EditorContent: return true;
        case KnownPathID::None: return false;
        case KnownPathID::Max: return false;
        default:
            ERROR_AND_DIE("UNSUPPORTED KNOWNPATHID")
        }
    }
    return false;
}

bool IsSystemPathId(const KnownPathID& pathid) noexcept {
    switch(pathid) {
    case KnownPathID::None: return false;
    case KnownPathID::GameConfig: return false;
    case KnownPathID::GameData: return false;
    case KnownPathID::GameFonts: return false;
    case KnownPathID::GameMaterials: return false;
    case KnownPathID::GameLogs: return false;
    case KnownPathID::EngineConfig: return false;
    case KnownPathID::EngineData: return false;
    case KnownPathID::EngineFonts: return false;
    case KnownPathID::EngineMaterials: return false;
    case KnownPathID::EngineLogs: return false;
    case KnownPathID::EditorContent: return false;
    case KnownPathID::Max: return false;
#if defined(PLATFORM_WINDOWS)
    case KnownPathID::Windows_AppDataRoaming: return true;
    case KnownPathID::Windows_AppDataLocal: return true;
    case KnownPathID::Windows_AppDataLocalLow: return true;
    case KnownPathID::Windows_ProgramFiles: return true;
    case KnownPathID::Windows_ProgramFilesx86: return true;
    case KnownPathID::Windows_ProgramFilesx64: return true;
    case KnownPathID::Windows_Documents: return true;
    case KnownPathID::Windows_CommonDocuments: return true;
    case KnownPathID::Windows_SavedGames: return true;
    case KnownPathID::Windows_UserProfile: return true;
    case KnownPathID::Windows_CommonProfile: return true;
    case KnownPathID::Windows_CurrentUserDesktop: return true;
#elif PLATFORM_LINUX
    case KnownPathID::Linux_RootUser: return true;
    case KnownPathID::Linux_Home: return true;
    case KnownPathID::Linux_Etc: return true;
    case KnownPathID::Linux_ConfigurationFiles: return true;
    case KnownPathID::Linux_Bin: return true;
    case KnownPathID::Linux_UserBinaries: return true;
    case KnownPathID::Linux_SBin: return true;
    case KnownPathID::Linux_SystemBinaries: return true;
    case KnownPathID::Linux_Dev: return true;
    case KnownPathID::Linux_DeviceFiles: return true;
    case KnownPathID::Linux_Proc: return true;
    case KnownPathID::Linux_ProcessInformation: return true;
    case KnownPathID::Linux_Var: return true;
    case KnownPathID::Linux_VariableFiles: return true;
    case KnownPathID::Linux_Usr: return true;
    case KnownPathID::Linux_UserPrograms: return true;
    case KnownPathID::Linux_UsrBin: return true;
    case KnownPathID::Linux_UserProgramsBinaries: return true;
    case KnownPathID::Linux_UsrSBin: return true;
    case KnownPathID::Linux_UserProgramsSystemBinaries: return true;
    case KnownPathID::Linux_Boot: return true;
    case KnownPathID::Linux_BootLoader: return true;
    case KnownPathID::Linux_Lib: return true;
    case KnownPathID::Linux_SystemLibraries: return true;
    case KnownPathID::Linux_Opt: return true;
    case KnownPathID::Linux_OptionalAddOnApps: return true;
    case KnownPathID::Linux_Mnt: return true;
    case KnownPathID::Linux_MountDirectory: return true;
    case KnownPathID::Linux_Media: return true;
    case KnownPathID::Linux_RemovableDevices: return true;
    case KnownPathID::Linux_Src: return true;
    case KnownPathID::Linux_ServiceData: return true;
#endif
    default:
        ERROR_AND_DIE("UNSUPPORTED KNOWNPATHID")
    }
}

std::filesystem::path GetKnownFolderPath(const KnownPathID& pathid) noexcept {
    namespace FS = std::filesystem;
    FS::path p{};
    if(!(IsSystemPathId(pathid) || IsContentPathId(pathid))) {
        return p;
    }
    if(pathid == KnownPathID::GameConfig) {
        p = GetWorkingDirectory() / FS::path{"Data/Config/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        } else {
            FileUtils::CreateFolders(GetWorkingDirectory() / FS::path{"Data/Config/"});
            p = GetKnownFolderPath(pathid);
        }
    } else if(pathid == KnownPathID::GameData) {
        p = GetWorkingDirectory() / FS::path{"Data/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::GameMaterials) {
        p = GetWorkingDirectory() / FS::path{"Data/Materials/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::GameFonts) {
        p = GetWorkingDirectory() / FS::path{"Data/Fonts/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::GameLogs) {
        p = GetWorkingDirectory() / FS::path{"Data/Logs/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::EngineConfig) {
        p = GetWorkingDirectory() / FS::path{"Engine/Config/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        } else {
            FileUtils::CreateFolders(GetWorkingDirectory() / FS::path{"Engine/Config/"});
            p = GetKnownFolderPath(pathid);
        }
    } else if(pathid == KnownPathID::EngineData) {
        p = GetWorkingDirectory() / FS::path{"Engine/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::EngineMaterials) {
        p = GetWorkingDirectory() / FS::path{"Engine/Materials/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::EngineFonts) {
        p = GetWorkingDirectory() / FS::path{"Engine/Fonts/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::EngineLogs) {
        p = GetWorkingDirectory() / FS::path{"Engine/Logs/"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        }
    } else if(pathid == KnownPathID::EditorContent) {
        p = GetWorkingDirectory() / FS::path{"Content"};
        if(FS::exists(p)) {
            p = FS::canonical(p);
        } else {
            FileUtils::CreateFolders(GetWorkingDirectory() / FS::path{ "Content" });
            p = GetKnownFolderPath(pathid);
        }
    } else {
#ifdef PLATFORM_WINDOWS
        {
            IKnownFolderManager* knownfoldermgr = nullptr;
            if(const auto kfmhr = ::CoCreateInstance(CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_IKnownFolderManager, reinterpret_cast<void**>(&knownfoldermgr)); SUCCEEDED(kfmhr)) {
                IKnownFolder* knownfolder = nullptr;
                if(const auto kfhr = knownfoldermgr->GetFolder(GetKnownPathIdForOS(pathid), &knownfolder); SUCCEEDED(kfhr)) {
                    PWSTR ppszPath = nullptr;
                    if(const auto hr_path = knownfolder->GetPath(0, &ppszPath); SUCCEEDED(hr_path)) {
                        p = FS::path(ppszPath);
                        ::CoTaskMemFree(ppszPath);
                        p = FS::canonical(p);
                        if(knownfolder) {
                            knownfolder->Release();
                            knownfolder = nullptr;
                        }
                    }
                    if(knownfoldermgr) {
                        knownfoldermgr->Release();
                        knownfoldermgr = nullptr;
                    }
                }
            }
        }
#endif
    }
    p.make_preferred();
    return p;
}

GUID GetKnownPathIdForOS(const KnownPathID& pathid) noexcept {
    switch(pathid) {
    case KnownPathID::Windows_AppDataRoaming:
        return FOLDERID_RoamingAppData;
    case KnownPathID::Windows_AppDataLocal:
        return FOLDERID_LocalAppData;
    case KnownPathID::Windows_AppDataLocalLow:
        return FOLDERID_LocalAppDataLow;
    case KnownPathID::Windows_ProgramFiles:
        return FOLDERID_ProgramFiles;
    case KnownPathID::Windows_ProgramFilesx86:
        return FOLDERID_ProgramFilesX86;
#if defined(_WIN64)
    case KnownPathID::Windows_ProgramFilesx64:
        return FOLDERID_ProgramFiles;
#elif defined(_WIN32)
    case KnownPathID::Windows_ProgramFilesx64:
        return FOLDERID_ProgramFiles;
#else
        ERROR_AND_DIE("Unknown known folder path id.");
        break;
#endif
    case KnownPathID::Windows_SavedGames:
        return FOLDERID_SavedGames;
    case KnownPathID::Windows_UserProfile:
        return FOLDERID_Profile;
    case KnownPathID::Windows_CommonProfile:
        return FOLDERID_Public;
    case KnownPathID::Windows_CurrentUserDesktop:
        return FOLDERID_Desktop;
    case KnownPathID::Windows_CommonDesktop:
        return FOLDERID_PublicDesktop;
    case KnownPathID::Windows_Documents:
        return FOLDERID_Documents;
        break;
    case KnownPathID::Windows_CommonDocuments:
        return FOLDERID_PublicDocuments;
        break;
    default:
        ERROR_AND_DIE("Unknown known folder path id.");
        break;
    }
}

std::filesystem::path GetExePath() noexcept {
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
            result = FS::canonical(result);
            result.make_preferred();
            return result;
        }
    }
    return result;
}

std::filesystem::path GetWorkingDirectory() noexcept {
    namespace FS = std::filesystem;
    return FS::current_path();
}

void SetWorkingDirectory(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    FS::current_path(p);
}

std::filesystem::path GetTempDirectory() noexcept {
    return std::filesystem::temp_directory_path();
}

bool HasDeletePermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    const auto parent_path = p.parent_path();
    const auto parent_status = FS::status(parent_path);
    const auto parent_perms = parent_status.permissions();
    if(FS::perms::none == (parent_perms & (FS::perms::owner_write | FS::perms::group_write | FS::perms::others_write))) {
        return false;
    }
    return true;
}

bool HasExecuteOrSearchPermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    if(FS::is_directory(p)) {
        return HasSearchPermissions(p);
    } else {
        return HasExecutePermissions(p);
    }
}

bool HasExecutePermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    if(FS::is_directory(p)) {
        return false;
    }
    const auto status = FS::status(p);
    const auto perms = status.permissions();
    if(FS::perms::none == (perms & (FS::perms::owner_exec | FS::perms::group_exec | FS::perms::others_exec))) {
        return false;
    }
    return true;
}

bool HasSearchPermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    if(!FS::is_directory(p)) {
        return false;
    }
    const auto parent_path = p.parent_path();
    const auto parent_status = FS::status(parent_path);
    const auto parent_perms = parent_status.permissions();
    if(FS::perms::none == (parent_perms & (FS::perms::owner_exec | FS::perms::group_exec | FS::perms::others_exec))) {
        return false;
    }
    return true;
}

bool HasWritePermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    const auto my_status = FS::status(p);
    const auto my_perms = my_status.permissions();
    if(FS::perms::none == (my_perms & (FS::perms::owner_write | FS::perms::group_write | FS::perms::others_write))) {
        return false;
    }
    return true;
}

bool HasReadPermissions(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    const auto my_status = FS::status(p);
    const auto my_perms = my_status.permissions();
    if(FS::perms::none == (my_perms & (FS::perms::owner_read | FS::perms::group_read | FS::perms::others_read))) {
        return false;
    }
    return true;
}

bool IsSafeWritePath(const std::filesystem::path& p) noexcept {
    //Check for any write permissions on the file and parent directory
    if(!(HasWritePermissions(p) || HasDeletePermissions(p))) {
        return false;
    }

    try {
        const auto is_in_working_dir = IsChildOf(p, GetWorkingDirectory());
        const auto is_in_gamedata_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::GameData));
        const auto is_in_enginedata_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::EngineData));
        const auto is_in_editorcontent_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::EditorContent));
        const auto is_temp_dir = IsChildOf(p, GetTempDirectory());
        const auto is_next_to_exe = IsSiblingOf(p, GetExePath());
        const auto is_known_OS_dir = false;

        const auto safe = is_in_working_dir || is_in_gamedata_dir || is_in_enginedata_dir || is_in_editorcontent_dir || is_temp_dir || is_next_to_exe || is_known_OS_dir;
        return safe;
    } catch(const std::filesystem::filesystem_error& e) {
        DebuggerPrintf("\nFilesystem Error:\nWhat: %s\nCode: %i\nPath1: %s\nPath2: %s\n", e.what(), e.code().value(), e.path1().string().c_str(), e.path2().string().c_str());
        return false;
    } catch(...) {
        DebuggerPrintf("\nUnspecified error trying to determine if path:\n%s\n is a safe write path.", p.string().c_str());
        return false;
    }
}

bool IsSafeReadPath(const std::filesystem::path& p) noexcept {
    namespace FS = std::filesystem;
    if(!FS::exists(p)) {
        return false;
    }
    //Check for any read permissions on the file and parent directory
    if(!(HasReadPermissions(p) || HasExecuteOrSearchPermissions(p))) {
        return false;
    }

    try {
        const auto is_in_working_dir = IsChildOf(p, GetWorkingDirectory());
        const auto is_in_gamedata_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::GameData));
        const auto is_in_enginedata_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::EngineData));
        const auto is_in_editorcontent_dir = IsChildOf(p, GetKnownFolderPath(KnownPathID::EditorContent));
        const auto is_next_to_exe = IsSiblingOf(p, GetExePath());
        const auto is_known_OS_dir = false;

        const auto safe = is_in_working_dir || is_in_gamedata_dir || is_in_enginedata_dir || is_in_editorcontent_dir || is_next_to_exe || is_known_OS_dir;
        return safe;
    } catch(const std::filesystem::filesystem_error& e) {
        const auto path1_str = e.path1().string();
        const auto path2_str = e.path1().string();
        DebuggerPrintf("\nFilesystem Error:\nWhat: %s\nCode: %i\nPath1: %s\nPath2: %s\n", e.what(), e.code().value(), path1_str.c_str(), path2_str.c_str());
        return false;
    } catch(...) {
        DebuggerPrintf("\nUnspecified error trying to determine if path:\n%s\n is a safe read path.", p.string().c_str());
        return false;
    }
}

bool IsParentOf(const std::filesystem::path& p, const std::filesystem::path& child) noexcept {
    namespace FS = std::filesystem;
    std::error_code ec{};
    if(const auto p_canon = FS::canonical(p, ec); !ec) {
        if(const auto child_canon = FS::canonical(child, ec); !ec) {
            for(auto iter = FS::recursive_directory_iterator{ p_canon }; iter != FS::recursive_directory_iterator{}; ++iter) {
                const auto& entry = *iter;
                const std::filesystem::path sub_p = entry.path();
                if(sub_p == child_canon) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool IsSiblingOf(const std::filesystem::path& p, const std::filesystem::path& sibling) noexcept {
    namespace FS = std::filesystem;
    std::error_code ec{};
    if(const auto my_parent_path = FS::canonical(p.parent_path(), ec); !ec) {
        if(const auto sibling_parent_path = FS::canonical(sibling.parent_path(), ec); !ec) {
            return my_parent_path == sibling_parent_path;
        }
    }
    return false;
}

bool IsChildOf(const std::filesystem::path& p, const std::filesystem::path& parent) noexcept {
    namespace FS = std::filesystem;
    std::error_code ec{};
    if(const auto parent_canon = FS::canonical(parent, ec); !ec) {
        if(const auto p_canon = FS::canonical(p, ec); !ec) {
            for(auto iter = FS::recursive_directory_iterator{parent_canon}; iter != FS::recursive_directory_iterator{}; iter = iter.increment(ec)) {
                if(ec) {
                    continue;
                }
                const auto& entry = *iter;
                const std::filesystem::path sub_p = entry.path();
                if(sub_p == p_canon) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::size_t CountFilesInFolders(const std::filesystem::path& folderpath, const std::string& validExtensionList /*= std::string{}*/, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    auto count = std::size_t{0u};
    const auto cb = [&count](const FS::path&) -> void { ++count; };
    ForEachFileInFolder(folderpath, validExtensionList, cb, recursive);
    return count;
}

std::vector<std::filesystem::path> GetAllPathsInFolders(const std::filesystem::path& folderpath, const std::string& validExtensionList /*= std::string{}*/, bool recursive /*= false*/) noexcept {
    namespace FS = std::filesystem;
    std::vector<FS::path> paths{};
    paths.reserve(CountFilesInFolders(folderpath));
    const auto add_path_cb = [&paths](const FS::path& p) { paths.push_back(p); };
    ForEachFileInFolder(folderpath, validExtensionList, add_path_cb, recursive);
    return paths;
}

void FileUtils::RemoveExceptMostRecentFiles(const std::filesystem::path& folderpath, std::size_t mostRecentCountToKeep, const std::string& validExtensionList /*= std::string{}*/) noexcept {
    if(!IsSafeWritePath(folderpath)) {
        return;
    }
    namespace FS = std::filesystem;
    if(mostRecentCountToKeep < CountFilesInFolders(folderpath, validExtensionList)) {
        auto paths = GetAllPathsInFolders(folderpath);
        const auto sort_pred = [](const FS::path& a, const FS::path& b) { return FS::last_write_time(a) > FS::last_write_time(b); };
        std::sort(std::begin(paths), std::end(paths), sort_pred);
        if(mostRecentCountToKeep > 0) {
            const auto erase_end = std::begin(paths) + mostRecentCountToKeep;
            paths.erase(std::begin(paths), erase_end);
        }
        for(auto& p : paths) {
            FS::remove(p);
        }
    }
}

} // namespace FileUtils