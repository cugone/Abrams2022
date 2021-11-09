#include "Engine/Core/StringUtils.hpp"

#include "Engine/Core/Rgba.hpp"
#include "Engine/Platform/Win.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"
#include "Engine/System/Cpu.hpp"
#include "Engine/System/System.hpp"

#include <algorithm>
#include <cstdarg>
#include <cwctype>
#include <locale>
#include <numeric>
#include <sstream>

const int STRINGF_STACK_LOCAL_TEMP_LENGTH = 2048;

namespace StringUtils {

std::string FormatWindowsMessage(unsigned long messageId) noexcept {
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
}

std::string FormatWindowsLastErrorMessage() noexcept {
    const auto err = ::GetLastError();
    return StringUtils::FormatWindowsMessage(err);
}

const std::string Stringf(const char* format, ...) noexcept {
    char textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, STRINGF_STACK_LOCAL_TEMP_LENGTH, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[STRINGF_STACK_LOCAL_TEMP_LENGTH - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    return std::string(textLiteral);
}

const std::string Stringf(const int maxLength, const char* format, ...) noexcept {
    char textLiteralSmall[STRINGF_STACK_LOCAL_TEMP_LENGTH];
    char* textLiteral = textLiteralSmall;
    if(maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        textLiteral = new char[maxLength];

    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[maxLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    std::string returnValue(textLiteral);
    if(maxLength > STRINGF_STACK_LOCAL_TEMP_LENGTH)
        delete[] textLiteral;

    return returnValue;
}

std::vector<std::string> Split(const std::string& string, char delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    std::size_t potential_count = 1 + std::count(string.begin(), string.end(), delim);
    std::vector<std::string> result;
    result.reserve(potential_count);
    std::string curString;

    std::stringstream ss;
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
        result.push_back(std::string{});
    }
    result.shrink_to_fit();
    return result;
}

std::vector<std::wstring> Split(const std::wstring& string, wchar_t delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    std::size_t potential_count = 1 + std::count(string.begin(), string.end(), delim);
    std::vector<std::wstring> result;
    result.reserve(potential_count);
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
    result.shrink_to_fit();
    return result;
}

std::vector<std::string> SplitOnUnquoted(const std::string& string, char delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    bool inQuote = false;
    std::vector<std::string> result{};
    std::size_t potential_count = 1u + std::count(std::begin(string), std::end(string), delim);
    result.reserve(potential_count);
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
    result.shrink_to_fit();
    return result;
}

std::vector<std::wstring> SplitOnUnquoted(const std::wstring& string, wchar_t delim /*= ','*/, bool skip_empty /*= true*/) noexcept {
    bool inQuote = false;
    std::vector<std::wstring> result{};
    std::size_t potential_count = 1u + std::count(std::begin(string), std::end(string), delim);
    result.reserve(potential_count);
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
    result.shrink_to_fit();
    return result;
}

std::pair<std::string, std::string> SplitOnFirst(const std::string& string, char delim) noexcept {
    auto eq_loc = string.find_first_of(delim);
    if(eq_loc != std::string::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(string, std::string{});
    }
}

std::pair<std::wstring, std::wstring> SplitOnFirst(const std::wstring& string, wchar_t delim) noexcept {
    auto eq_loc = string.find_first_of(delim);
    if(eq_loc != std::wstring::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(string.substr(0, eq_loc), std::wstring{});
    }
}

std::pair<std::string, std::string> SplitOnLast(const std::string& string, char delim) noexcept {
    auto eq_loc = string.find_last_of(delim);
    if(eq_loc != std::string::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(std::string{}, string.substr(eq_loc + 1));
    }
}

std::pair<std::wstring, std::wstring> SplitOnLast(const std::wstring& string, wchar_t delim) noexcept {
    auto eq_loc = string.find_last_of(delim);
    if(eq_loc != std::wstring::npos) {
        return std::make_pair(string.substr(0, eq_loc), string.substr(eq_loc + 1));
    } else {
        return std::make_pair(std::wstring{}, string.substr(eq_loc + 1));
    }
}

std::string Join(const std::vector<std::string>& strings, char delim, bool skip_empty /*= true*/) noexcept {
    auto acc_op = [](const std::size_t& a, const std::string& b) -> std::size_t { return a + static_cast<std::size_t>(1u) + b.size(); };
    auto total_size = std::accumulate(std::begin(strings), std::end(strings), static_cast<std::size_t>(0u), acc_op);
    std::string result{};
    result.reserve(total_size);
    for(auto iter = strings.begin(); iter != strings.end(); ++iter) {
        if(skip_empty && (*iter).empty())
            continue;
        result += (*iter);
        if(iter != strings.end() - 1) {
            result.push_back(delim);
        }
    }
    result.shrink_to_fit();
    return result;
}

std::string Join(const std::vector<std::string>& strings, bool skip_empty /*= true*/) noexcept {
    auto acc_op = [](const std::size_t& a, const std::string& b) { return a + b.size(); };
    std::size_t total_size = std::accumulate(std::begin(strings), std::end(strings), static_cast<std::size_t>(0u), acc_op);
    std::string result;
    result.reserve(total_size);
    for(const auto& string : strings) {
        if(skip_empty && string.empty())
            continue;
        result += string;
    }
    result.shrink_to_fit();
    return result;
}

std::wstring Join(const std::vector<std::wstring>& strings, wchar_t delim, bool skip_empty /*= true*/) noexcept {
    auto acc_op = [](const std::size_t& a, const std::wstring& b) -> std::size_t { return a + static_cast<std::size_t>(1u) + b.size(); };
    auto total_size = std::accumulate(std::begin(strings), std::end(strings), static_cast<std::size_t>(0u), acc_op);
    std::wstring result{};
    result.reserve(total_size);
    for(auto iter = strings.begin(); iter != strings.end(); ++iter) {
        if(skip_empty && (*iter).empty())
            continue;
        result += (*iter);
        if(iter != strings.end() - 1) {
            result.push_back(delim);
        }
    }
    result.shrink_to_fit();
    return result;
}

std::wstring Join(const std::vector<std::wstring>& strings, bool skip_empty /*= true*/) noexcept {
    auto acc_op = [](const std::size_t& a, const std::wstring& b) { return a + b.size(); };
    std::size_t total_size = std::accumulate(std::begin(strings), std::end(strings), static_cast<std::size_t>(0u), acc_op);
    std::wstring result;
    result.reserve(total_size);
    for(const auto& string : strings) {
        if(skip_empty && string.empty())
            continue;
        result += string;
    }
    result.shrink_to_fit();
    return result;
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
    std::unique_ptr<char[]> buf = nullptr;
    auto buf_size = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicode_string.data(), -1, buf.get(), 0, nullptr, nullptr));
    if(!buf_size) {
        return {};
    }
    buf = std::make_unique<char[]>(buf_size * sizeof(char));
    if(!buf) {
        return {};
    }
    buf_size = static_cast<std::size_t>(::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, unicode_string.data(), -1, buf.get(), static_cast<int>(buf_size), nullptr, nullptr));
    if(!buf_size) {
        return {};
    }
    std::string mb_string{};
    mb_string.assign(buf.get(), buf_size - 1);
    return mb_string;
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
    auto found_loc = string.find(start);
    return found_loc != std::string::npos && found_loc == 0;
}

bool StartsWith(const std::wstring& string, const std::wstring& start) noexcept {
    auto found_loc = string.find(start);
    return found_loc != std::wstring::npos && found_loc == 0;
}

bool StartsWith(const std::string& string, char start) noexcept {
    return string.front() == start;
}

bool StartsWith(const std::wstring& string, wchar_t start) noexcept {
    return string.front() == start;
}

bool EndsWith(const std::string& string, const std::string& end) noexcept {
    auto found_loc = string.rfind(end);
    return found_loc != std::string::npos && found_loc == string.size() - end.size();
}

bool EndsWith(const std::wstring& string, const std::wstring& end) noexcept {
    auto found_loc = string.rfind(end);
    return found_loc != std::wstring::npos && found_loc == string.size() - end.size();
}

bool EndsWith(const std::string& string, char end) noexcept {
    return string.back() == end;
}

bool EndsWith(const std::wstring& string, wchar_t end) noexcept {
    return string.back() == end;
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

std::vector<std::size_t> FindAll(std::string string, const char c) noexcept {
    std::vector<std::size_t> results{};
    std::size_t offset = 0;
    while((offset = string.find(c, offset)) != std::string::npos) {
        results.push_back(offset++);
    }
    return results;
}

std::vector<std::size_t> FindAll(std::string string, const std::string& sequence) noexcept {
    std::vector<std::size_t> results{};
    std::size_t offset = 0;
    while((offset = string.find(sequence, offset)) != std::string::npos) {
        results.push_back(offset);
    }
    return results;
}

std::vector<std::size_t> FindAll(std::wstring string, const wchar_t c) noexcept {
    std::vector<std::size_t> results{};
    std::size_t offset = 0;
    while((offset = string.find(c, offset)) != std::wstring::npos) {
        results.push_back(offset++);
    }
    return results;
}

std::vector<std::size_t> FindAll(std::wstring string, const std::wstring& sequence) noexcept {
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