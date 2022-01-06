#pragma once

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <any>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace FileUtils {

enum class KnownPathID {
    None,
    Windows_AppDataRoaming,
    Windows_AppDataLocal,
    Windows_AppDataLocalLow,
    Windows_ProgramFiles,
    Windows_ProgramFilesx86,
    Windows_ProgramFilesx64,
    Windows_Documents,
    Windows_CommonDocuments,
    Windows_SavedGames,
    Windows_UserProfile,
    Windows_CommonProfile,
    Windows_CurrentUserDesktop,
    Windows_CommonDesktop,
    Linux_RootUser,
    Linux_Home,
    Linux_Etc,
    Linux_ConfigurationFiles = Linux_Etc,
    Linux_Bin,
    Linux_UserBinaries = Linux_Bin,
    Linux_SBin,
    Linux_SystemBinaries = Linux_SBin,
    Linux_Dev,
    Linux_DeviceFiles = Linux_Dev,
    Linux_Proc,
    Linux_ProcessInformation = Linux_Proc,
    Linux_Var,
    Linux_VariableFiles = Linux_Var,
    Linux_Usr,
    Linux_UserPrograms = Linux_Usr,
    Linux_UsrBin,
    Linux_UserProgramsBinaries = Linux_UsrBin,
    Linux_UsrSBin,
    Linux_UserProgramsSystemBinaries = Linux_UsrSBin,
    Linux_Boot,
    Linux_BootLoader = Linux_Boot,
    Linux_Lib,
    Linux_SystemLibraries = Linux_Lib,
    Linux_Opt,
    Linux_OptionalAddOnApps = Linux_Opt,
    Linux_Mnt,
    Linux_MountDirectory = Linux_Mnt,
    Linux_Media,
    Linux_RemovableDevices = Linux_Media,
    Linux_Src,
    Linux_ServiceData = Linux_Src,
    GameData,
    GameConfig,
    GameFonts,
    GameMaterials,
    GameLogs,
    EngineConfig,
    EngineData,
    EngineFonts,
    EngineMaterials,
    EngineLogs,
    EditorContent,
    Max
};

[[nodiscard]] bool WriteBufferToFile(const std::any& buffer, std::size_t size, std::filesystem::path filepath) noexcept;
[[nodiscard]] bool WriteBufferToFile(const std::string& buffer, std::filesystem::path filepath) noexcept;
[[nodiscard]] std::optional<std::vector<uint8_t>> ReadBinaryBufferFromFile(std::filesystem::path filepath) noexcept;
[[nodiscard]] std::optional<std::string> ReadStringBufferFromFile(std::filesystem::path filepath) noexcept;
[[nodiscard]] std::optional<std::string> ReadSomeBinaryBufferFromFile(std::filesystem::path filepath, std::size_t pos, std::size_t count = 0u) noexcept;
[[nodiscard]] std::optional<std::string> ReadSomeBinaryBufferFromFile(std::ifstream& ifs, std::streampos pos, std::streamsize count = 0u) noexcept;
[[nodiscard]] std::optional<std::string> ReadSomeStringBufferFromFile(std::filesystem::path filepath, std::size_t pos, std::size_t count = 0u) noexcept;
[[nodiscard]] std::optional<std::string> ReadSomeStringBufferFromFile(std::ifstream& ifs, std::streampos pos, std::streamsize count = 0u) noexcept;
bool CreateFolders(const std::filesystem::path& filepath) noexcept;
[[nodiscard]] bool IsSystemPathId(const KnownPathID& pathid) noexcept;
[[nodiscard]] bool IsContentPathId(const KnownPathID& pathid) noexcept;
[[nodiscard]] std::filesystem::path GetKnownFolderPath(const KnownPathID& pathid) noexcept;
[[nodiscard]] std::filesystem::path GetExePath() noexcept;
[[nodiscard]] std::filesystem::path GetWorkingDirectory() noexcept;
[[nodiscard]] void SetWorkingDirectory(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool IsSafeWritePath(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool IsSafeReadPath(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasWritePermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasReadPermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasDeletePermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasExecuteOrSearchPermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasExecutePermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool HasSearchPermissions(const std::filesystem::path& p) noexcept;
[[nodiscard]] bool IsParentOf(const std::filesystem::path& p, const std::filesystem::path& child) noexcept;
[[nodiscard]] bool IsSiblingOf(const std::filesystem::path& p, const std::filesystem::path& sibling) noexcept;
[[nodiscard]] bool IsChildOf(const std::filesystem::path& p, const std::filesystem::path& parent) noexcept;

[[nodiscard]] std::size_t CountFilesInFolders(const std::filesystem::path& folderpath, const std::string& validExtensionList = std::string{}, bool recursive = false) noexcept;
void RemoveExceptMostRecentFiles(const std::filesystem::path& folderpath, std::size_t mostRecentCountToKeep, const std::string& validExtensionList = std::string{}) noexcept;
[[nodiscard]] std::vector<std::filesystem::path> GetAllPathsInFolders(const std::filesystem::path& folderpath, const std::string& validExtensionList = std::string{}, bool recursive = false) noexcept;

namespace detail {

template<typename DirectoryIteratorType, typename Callable>
void ForEachFileInFolders(const std::filesystem::path& preferred_folderpath, const std::vector<std::string>& validExtensions, Callable&& callback) noexcept {
    if(validExtensions.empty()) {
        std::for_each(DirectoryIteratorType{preferred_folderpath}, DirectoryIteratorType{},
                      [&callback](const std::filesystem::directory_entry& entry) {
                          const auto& cur_path = entry.path();
                          bool is_file = std::filesystem::is_regular_file(cur_path);
                          if(is_file) {
                              std::invoke(callback, cur_path);
                          }
                      });
        return;
    }
    std::for_each(DirectoryIteratorType{preferred_folderpath}, DirectoryIteratorType{},
                  [&validExtensions, &callback](const std::filesystem::directory_entry& entry) {
                      const auto& cur_path = entry.path();
                      bool is_file = std::filesystem::is_regular_file(cur_path);
                      std::string my_extension = StringUtils::ToLowerCase(cur_path.extension().string());
                      if(is_file) {
                          bool valid_file_by_extension = std::find(std::begin(validExtensions), std::end(validExtensions), my_extension) != std::end(validExtensions);
                          if(valid_file_by_extension) {
                              std::invoke(callback, cur_path);
                          }
                      }
                  });
}
} // namespace detail

template<typename Callable>
void ForEachFileInFolder(
const std::filesystem::path& folderpath, const std::string& validExtensionList = std::string{}, Callable&& callback = [](const std::filesystem::path&) {}, bool recursive = false) noexcept {
    namespace FS = std::filesystem;
    const auto exists = FS::exists(folderpath);
    if(!exists) {
        return;
    }
    auto preferred_folderpath = folderpath;
    {
        std::error_code ec{};
        preferred_folderpath = FS::canonical(preferred_folderpath, ec);
        if(ec) {
            return;
        }
    }
    preferred_folderpath.make_preferred();
    const auto is_directory = FS::is_directory(preferred_folderpath);
    const auto is_folder = exists && is_directory;
    if(!is_folder) {
        return;
    }
    const auto validExtensions = StringUtils::Split(StringUtils::ToLowerCase(validExtensionList));
    if(!recursive) {
        detail::ForEachFileInFolders<FS::directory_iterator>(preferred_folderpath, validExtensions, callback);
    } else {
        detail::ForEachFileInFolders<FS::recursive_directory_iterator>(preferred_folderpath, validExtensions, callback);
    }
}

} // namespace FileUtils