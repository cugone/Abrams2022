#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace StringUtils {

[[nodiscard]] std::string FormatWindowsMessage(unsigned long messageId) noexcept;
[[nodiscard]] std::string FormatWindowsLastErrorMessage() noexcept;

[[nodiscard]] std::vector<std::string> Split(std::string string, char delim = ',', bool skip_empty = true) noexcept;
[[nodiscard]] std::vector<std::wstring> Split(std::wstring string, wchar_t delim = ',', bool skip_empty = true) noexcept;
[[nodiscard]] std::vector<std::string> SplitOnUnquoted(std::string string, char delim = ',', bool skip_empty = true) noexcept;
[[nodiscard]] std::vector<std::wstring> SplitOnUnquoted(std::wstring string, wchar_t delim = ',', bool skip_empty = true) noexcept;
[[nodiscard]] std::pair<std::string, std::string> SplitOnFirst(std::string string, char delim) noexcept;
[[nodiscard]] std::pair<std::wstring, std::wstring> SplitOnFirst(std::wstring string, wchar_t delim) noexcept;
[[nodiscard]] std::pair<std::string, std::string> SplitOnLast(std::string string, char delim) noexcept;
[[nodiscard]] std::pair<std::wstring, std::wstring> SplitOnLast(std::wstring string, wchar_t delim) noexcept;

[[nodiscard]] std::string ToUpperCase(std::string string) noexcept;
[[nodiscard]] std::wstring ToUpperCase(std::wstring string) noexcept;

[[nodiscard]] std::string ToLowerCase(std::string string) noexcept;
[[nodiscard]] std::wstring ToLowerCase(std::wstring string) noexcept;

[[nodiscard]] std::string ConvertUnicodeToMultiByte(const std::wstring& unicode_string) noexcept;
[[nodiscard]] std::wstring ConvertMultiByteToUnicode(const std::string& multi_byte_string) noexcept;

[[nodiscard]] bool StartsWith(const std::string& string, const std::string& start) noexcept;
[[nodiscard]] bool StartsWith(const std::wstring& string, const std::wstring& start) noexcept;
[[nodiscard]] bool StartsWith(const std::string& string, char start) noexcept;
[[nodiscard]] bool StartsWith(const std::wstring& string, wchar_t start) noexcept;

[[nodiscard]] std::string ReplaceAll(std::string string, const std::string& from, const std::string& to) noexcept;
[[nodiscard]] std::wstring ReplaceAll(std::wstring string, const std::wstring& from, const std::wstring& to) noexcept;

[[nodiscard]] std::vector<std::size_t> FindAll(std::string string, const char c) noexcept;
[[nodiscard]] std::vector<std::size_t> FindAll(std::wstring string, const wchar_t c) noexcept;
[[nodiscard]] std::vector<std::size_t> FindAll(std::string string, const std::string& sequence) noexcept;
[[nodiscard]] std::vector<std::size_t> FindAll(std::wstring string, const std::wstring& sequence) noexcept;

[[nodiscard]] std::string TrimWhitespace(const std::string& string) noexcept;
[[nodiscard]] std::wstring TrimWhitespace(const std::wstring& string) noexcept;

[[nodiscard]] constexpr const uint32_t FourCC(const char* id) noexcept {
    return static_cast<uint32_t>((((id[0] << 24) & 0xFF000000) | ((id[1] << 16) & 0x00FF0000) | ((id[2] << 8) & 0x0000FF00) | ((id[3] << 0) & 0x000000FF)));
}

void CopyFourCC(char* destFCC, const char* srcFCC) noexcept;
[[nodiscard]] std::string FourCCToString(const char* id) noexcept;

namespace Encryption {

//NOT USEFUL AS TRUE ENCRYPTION!! DO NOT USE IF SERIOUS ENCRYPTION IS NEEDED!!!
[[nodiscard]] std::string ROT13(std::string_view text) noexcept;

//NOT USEFUL AS TRUE ENCRYPTION!! DO NOT USE IF SERIOUS ENCRYPTION IS NEEDED!!!
[[nodiscard]] std::string CaesarShift(std::string_view text, bool encode = true) noexcept;

//NOT USEFUL AS TRUE ENCRYPTION!! DO NOT USE IF SERIOUS ENCRYPTION IS NEEDED!!!
[[nodiscard]] std::string ShiftCipher(int key, std::string_view text) noexcept;

} // namespace Encryption

} // namespace StringUtils
