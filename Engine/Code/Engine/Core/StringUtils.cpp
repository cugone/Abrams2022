#include "Engine/Core/StringUtils.hpp"

#include "Engine/Core/Rgba.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/System/Cpu.hpp"
#include "Engine/System/System.hpp"

#include <cstdarg>
#include <cwctype>
#include <format>
#include <numeric>
#include <sstream>

namespace StringUtils {

std::string FormatWindowsMessage(unsigned long messageId) noexcept {
#ifdef PLATFORM_WINDOWS
    HRESULT hresult = static_cast<HRESULT>(messageId);
    LPSTR errorText = nullptr;
    DWORD result = ::FormatMessageA(
    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, hresult,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    reinterpret_cast<LPSTR>(&errorText), 0, nullptr);
    if(result > 0) {
        std::string err{errorText ? errorText : ""};
        ::LocalFree(errorText);
        errorText = nullptr;
        return err;
    }
    return {"Trying to Format unknown error."};
#else
    return std::format("Windows Message ID: {}", messageId);
#endif
}

std::string FormatWindowsLastErrorMessage() noexcept {
#ifdef PLATFORM_WINDOWS
    return StringUtils::FormatWindowsMessage(::GetLastError());
#else
    return {};
#endif
}

std::vector<std::string> Split(std::string string, char delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    std::vector<std::string> result{};
    result.reserve(1u + std::count(string.begin(), string.end(), delim));

    std::stringstream ss;
    ss.str(string);
    ss.seekg(0);
    ss.seekp(0);
    ss.clear();

    std::string curString;
    while(std::getline(ss, curString, delim)) {
        if(skip_empty && curString.empty()) {
            continue;
        }
        result.push_back(curString);
    }
    if(!skip_empty && ss.eof() && string.back() == delim) {
        result.push_back(std::string{});
    }
    return result;
}

std::vector<std::wstring> Split(std::wstring string, wchar_t delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    std::vector<std::wstring> result{};
    result.reserve(1u + std::count(string.begin(), string.end(), delim));
    std::wstring curString;

    std::wstringstream ss;
    ss.str(string);
    ss.seekg(0);
    ss.seekp(0);
    ss.clear();

    while(std::getline(ss, curString, delim)) {
        if(skip_empty && curString.empty())
            continue;
        result.push_back(curString);
    }
    if(!skip_empty && ss.eof() && string.back() == delim) {
        result.push_back(std::wstring{});
    }
    return result;
}

std::vector<std::string> SplitOnUnquoted(std::string string, char delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    bool inQuote = false;
    std::vector<std::string> result{};
    result.reserve(1u + std::count(std::begin(string), std::end(string), delim));
    auto start = std::begin(string);
    auto end = std::end(string);
    for(auto iter = std::begin(string); iter != std::end(string); /* DO NOTHING */) {
        if(*iter == '"') {
            inQuote = !inQuote;
            ++iter;
            continue;
        }
        if(!inQuote) {
            if(*iter == delim) {
                end = iter++;
                std::string s(start, end);
                if(skip_empty && s.empty()) {
                    start = iter;
                    continue;
                }
                result.push_back(std::string(start, end));
                start = iter;
                continue;
            }
        }
        ++iter;
    }
    end = std::end(string);
    auto last_s = std::string(start, end);
    if(!(skip_empty && last_s.empty())) {
        result.push_back(std::string(start, end));
    }
    return result;
}

std::vector<std::wstring> SplitOnUnquoted(std::wstring string, wchar_t delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    bool inQuote = false;
    std::vector<std::wstring> result{};
    result.reserve(1u + std::count(std::begin(string), std::end(string), delim));
    auto start = std::begin(string);
    auto end = std::end(string);
    for(auto iter = std::begin(string); iter != std::end(string); /* DO NOTHING */) {
        if(*iter == '"') {
            inQuote = !inQuote;
            ++iter;
            continue;
        }
        if(!inQuote) {
            if(*iter == delim) {
                end = iter++;
                std::wstring s(start, end);
                if(skip_empty && s.empty()) {
                    start = iter;
                    continue;
                }
                result.push_back(std::wstring(start, end));
                start = iter;
                continue;
            }
        }
        ++iter;
    }
    end = std::end(string);
    auto last_s = std::wstring(start, end);
    if(!(skip_empty && last_s.empty())) {
        result.push_back(std::wstring(start, end));
    }
    return result;
}

std::pair<std::string, std::string> SplitOnFirst(std::string string, char delim) noexcept {
    auto eq_loc = string.find_first_of(delim);
    if(eq_loc != std::string::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(string.substr(0, eq_loc), std::string{});
    }
}

std::pair<std::wstring, std::wstring> SplitOnFirst(std::wstring string, wchar_t delim) noexcept {
    auto eq_loc = string.find_first_of(delim);
    if(eq_loc != std::wstring::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(string.substr(0, eq_loc), std::wstring{});
    }
}

std::pair<std::string, std::string> SplitOnLast(std::string string, char delim) noexcept {
    auto eq_loc = string.find_last_of(delim);
    if(eq_loc != std::string::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(std::string{}, string.substr(eq_loc + 1));
    }
}

std::pair<std::wstring, std::wstring> SplitOnLast(std::wstring string, wchar_t delim) noexcept {
    auto eq_loc = string.find_last_of(delim);
    if(eq_loc != std::wstring::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(std::wstring{}, string.substr(eq_loc + 1));
    }
}

std::string ToUpperCase(std::string string) noexcept {
    std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) -> unsigned char { return std::toupper(c, std::locale("")); });
    return string;
}

std::wstring ToUpperCase(std::wstring string) noexcept {
    std::transform(string.begin(), string.end(), string.begin(), [](wchar_t c) -> wchar_t { return std::towupper(static_cast<std::wint_t>(c)); });
    return string;
}

std::string ToLowerCase(std::string string) noexcept {
    std::transform(string.begin(), string.end(), string.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c, std::locale("")); });
    return string;
}

std::wstring ToLowerCase(std::wstring string) noexcept {
    std::transform(string.begin(), string.end(), string.begin(), [](wchar_t c) -> wchar_t { return std::towlower(static_cast<std::wint_t>(c)); });
    return string;
}

std::string ConvertUnicodeToMultiByte(const std::wstring& unicode_string) noexcept {
    if(unicode_string.empty()) {
        return {};
    }
    if(auto buf_size = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicode_string.data(), -1, nullptr, 0, nullptr, nullptr)); !buf_size) {
        return {};
    } else {
        if(std::unique_ptr<char[]> buf = std::make_unique<char[]>(buf_size * sizeof(char)); !buf) {
            return {};
        } else {
            if(buf_size = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicode_string.data(), -1, buf.get(), static_cast<int>(buf_size), nullptr, nullptr)); !buf_size) {
                return {};
            } else {
                std::string mb_string{};
                mb_string.assign(buf.get(), buf_size - 1);
                return mb_string;
            }
        }
    }
}

std::wstring ConvertMultiByteToUnicode(const std::string& multi_byte_string) noexcept {
    if(multi_byte_string.empty()) {
        return {};
    }
    std::unique_ptr<wchar_t[]> buf = nullptr;
    auto buf_size = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, multi_byte_string.data(), -1, buf.get(), 0));
    if(!buf_size) {
        return {};
    }
    buf = std::make_unique<wchar_t[]>(buf_size * sizeof(wchar_t));
    if(!buf) {
        return {};
    }
    buf_size = static_cast<std::size_t>(::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, multi_byte_string.data(), -1, buf.get(), static_cast<int>(buf_size)));
    if(!buf_size) {
        return {};
    }
    std::wstring unicode_string{};
    unicode_string.assign(buf.get(), buf_size - 1);
    return unicode_string;
}

bool StartsWith(const std::string& string, const std::string& start) noexcept {
    return string.starts_with(start);
}

bool StartsWith(const std::wstring& string, const std::wstring& start) noexcept {
    return string.starts_with(start);
}

bool StartsWith(const std::string& string, char start) noexcept {
    return string.starts_with(start);
}

bool StartsWith(const std::wstring& string, wchar_t start) noexcept {
    return string.starts_with(start);
}

const bool Contains(const std::string& string, std::string_view sv) noexcept {
    return string.find(sv) != std::string::npos;
}

const bool Contains(const std::string& string, char ch) noexcept {
    return string.find(ch) != std::string::npos;
}

const bool Contains(const std::string& string, const char* s) noexcept {
    return string.find(s) != std::string::npos;
}

const bool Contains(const std::wstring& string, std::wstring_view sv) noexcept {
    return string.find(sv) != std::wstring::npos;
}

const bool Contains(const std::wstring& string, wchar_t ch) noexcept {
    return string.find(ch) != std::wstring::npos;
}

const bool Contains(const std::wstring& string, const wchar_t* s) noexcept {
    return string.find(s) != std::wstring::npos;
}

//From https://stackoverflow.com/a/3418285/421178
std::string ReplaceAll(std::string string, const std::string& from, const std::string& to) noexcept {
    if(from.empty()) {
        return string;
    }
    std::size_t start_pos = 0;
    while((start_pos = string.find(from, start_pos)) != std::string::npos) {
        string.replace(start_pos, from.size(), to);
        start_pos += to.size(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
    return string;
}

std::wstring ReplaceAll(std::wstring string, const std::wstring& from, const std::wstring& to) noexcept {
    if(from.empty()) {
        return string;
    }
    std::size_t start_pos = 0;
    while((start_pos = string.find(from, start_pos)) != std::wstring::npos) {
        string.replace(start_pos, from.size(), to);
        start_pos += to.size(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
    return string;
}

std::string RemoveAll(std::string string, const std::string& what) noexcept {
    return ReplaceAll(string, what, "");
}

std::wstring RemoveAll(std::wstring string, const std::wstring& what) noexcept {
    return ReplaceAll(string, what, L"");
}

std::string RemoveAllWhitespace(std::string string) noexcept {
    if(string.empty()) {
        return string;
    }
    string = RemoveAll(string, " ");
    string = RemoveAll(string, "\r");
    string = RemoveAll(string, "\n");
    string = RemoveAll(string, "\t");
    string = RemoveAll(string, "\v");
    string = RemoveAll(string, "\f");
    return string;
}

std::wstring RemoveAllWhitespace(std::wstring string) noexcept {
    if(string.empty()) {
        return string;
    }
    string = RemoveAll(string, L" ");
    string = RemoveAll(string, L"\r");
    string = RemoveAll(string, L"\n");
    string = RemoveAll(string, L"\t");
    string = RemoveAll(string, L"\v");
    string = RemoveAll(string, L"\f");
    return string;
}

std::vector<std::size_t> FindAll(std::string string, const char c) noexcept {
    if(string.empty()) {
        return std::vector<std::size_t>{};
    }
    std::vector<std::size_t> results{};
    if(const std::size_t count = std::count(std::cbegin(string), std::cend(string), c); count) {
        results.reserve(count);
        std::size_t offset = 0;
        while((offset = string.find(c, offset)) != std::string::npos) {
            results.push_back(offset++);
        }
    }
    return results;
}

std::vector<std::size_t> FindAll(std::string string, const std::string& sequence) noexcept {
    if(string.empty() || sequence.empty()) {
        return {};
    }
    std::vector<std::size_t> results;
    std::size_t offset = 0;
    while((offset = string.find(sequence, offset)) != std::string::npos) {
        results.push_back(offset);
    }
    return results;
}

std::vector<std::size_t> FindAll(std::wstring string, const wchar_t c) noexcept {
    if(string.empty()) {
        return {};
    }
    std::vector<std::size_t> results{};
    if(const std::size_t count = std::count(std::cbegin(string), std::cend(string), c); count) {
        results.reserve(count);
        std::size_t offset = 0;
        while((offset = string.find(c, offset)) != std::wstring::npos) {
            results.push_back(offset++);
        }
    }
    return results;
}

std::vector<std::size_t> FindAll(std::wstring string, const std::wstring& sequence) noexcept {
    if(string.empty() || sequence.empty()) {
        return {};
    }
    std::vector<std::size_t> results{};
    std::size_t offset = 0;
    while((offset = string.find(sequence, offset)) != std::wstring::npos) {
        results.push_back(offset);
    }
    return results;
}

std::string TrimWhitespace(const std::string& string) noexcept {
    auto first_non_wspace = string.find_first_not_of(" \r\n\t\v\f");
    if(first_non_wspace == std::string::npos) {
        return {};
    }
    auto last_non_wspace = string.find_last_not_of(" \r\n\t\v\f");
    return string.substr(first_non_wspace, last_non_wspace - first_non_wspace + 1);
}

std::wstring TrimWhitespace(const std::wstring& string) noexcept {
    auto first_non_wspace = string.find_first_not_of(L" \r\n\t\v\f");
    if(first_non_wspace == std::wstring::npos) {
        return {};
    }
    auto last_non_wspace = string.find_last_not_of(L" \r\n\t\v\f");
    return string.substr(first_non_wspace, last_non_wspace - first_non_wspace + 1);
}

void CopyFourCC(char* destFCC, const char* srcFCC) noexcept {
    destFCC[0] = srcFCC[0];
    destFCC[1] = srcFCC[1];
    destFCC[2] = srcFCC[2];
    destFCC[3] = srcFCC[3];
}

std::string FourCCToString(const char* id) noexcept {
    return std::string{id[0], id[1], id[2], id[3]};
}

namespace Encryption {

std::string ROT13(std::string_view text) noexcept {
    return ShiftCipher(13, text);
}

std::string CaesarShift(std::string_view text, bool encode /*= true*/) noexcept {
    return ShiftCipher(encode ? 3 : -3, text);
}

std::string ShiftCipher(int key, std::string_view text) noexcept {
    const auto shiftcipher = [key](unsigned char a) {
        const bool lower = 'a' <= a && a <= 'z';
        const bool upper = 'A' <= a && a <= 'Z';
        const char base = lower ? 'a' : upper ? 'A'
                                              : 0;
        if(!base) {
            return a;
        }
        int shift_result = (a - base + key) % 26;
        if(shift_result < 0) {
            shift_result += 26;
        }
        if(25 < shift_result) {
            shift_result -= 26;
        }
        return static_cast<unsigned char>(static_cast<char>(base + shift_result));
    };
    std::string cipher(text.size(), '\0');
    std::transform(std::begin(text), std::end(text), std::begin(cipher), shiftcipher);
    return cipher;
}

} // namespace Encryption

} // namespace StringUtils