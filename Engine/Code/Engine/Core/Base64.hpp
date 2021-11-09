#pragma once

#include <istream>
#include <string>
#include <vector>

namespace FileUtils::Base64 {

namespace detail {
static std::vector<char> base64encodingtable = {
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
static char base64paddingchar = '=';
} // namespace detail

[[nodiscard]] std::string Encode(std::istream& input) noexcept;
[[nodiscard]] std::string Encode(const std::string& input) noexcept;
[[nodiscard]] std::string Encode(const std::vector<unsigned char>& input) noexcept;

[[nodiscard]] std::string Decode(std::istream& input) noexcept;
[[nodiscard]] std::string Decode(const std::string& input) noexcept;
void Decode(const std::string& input, std::vector<unsigned char>& output) noexcept;
namespace detail {
[[nodiscard]] std::string Encode(std::istream& input, std::size_t size) noexcept;
[[nodiscard]] std::string Decode(std::istream& input, std::size_t size) noexcept;
} // namespace detail

} // namespace FileUtils::Base64