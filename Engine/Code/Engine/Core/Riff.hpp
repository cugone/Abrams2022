#pragma once

#include "Engine/Core/StringUtils.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace FileUtils {

namespace RiffChunkID {
constexpr const uint32_t RIFF = StringUtils::FourCC("RIFF");
constexpr const uint32_t LIST = StringUtils::FourCC("LIST");
constexpr const uint32_t WAVE = StringUtils::FourCC("WAVE");
constexpr const uint32_t INFO = StringUtils::FourCC("INFO");
constexpr const uint32_t AVI = StringUtils::FourCC("AVI ");
[[nodiscard]] constexpr const bool IsValid(const char* id) noexcept;
} // namespace RiffChunkID

class Riff {
public:
    static constexpr const unsigned int RIFF_SUCCESS = 0;
    static constexpr const unsigned int RIFF_ERROR_NOT_A_RIFF = 1;
    static constexpr const unsigned int RIFF_ERROR_INVALID_RIFF = 2;
    static constexpr const unsigned int RIFF_ERROR_INVALID_ARGUMENT = 3;

    struct RiffHeader {
        char fourcc[4] = {0};
        uint32_t length = 0u;
    };
    struct RiffSubChunk {
        char fourcc[4] = {0};
        std::size_t subdata_length{0};
        std::unique_ptr<uint8_t[]> subdata{};
    };
    struct RiffChunk {
        RiffHeader header{};
        std::unique_ptr<RiffSubChunk> data{};
    };

    [[nodiscard]] RiffChunk* GetNextChunk() const noexcept;
    [[nodiscard]] unsigned int Load(std::filesystem::path filename) noexcept;
    [[nodiscard]] unsigned int Load(const std::vector<unsigned char>& data) noexcept;
    [[nodiscard]] static std::optional<std::unique_ptr<Riff::RiffChunk>> ReadListChunk(std::stringstream& stream) noexcept;

protected:
private:
    [[nodiscard]] bool ParseDataIntoChunks(std::vector<unsigned char>& buffer) noexcept;

    std::vector<std::unique_ptr<RiffChunk>> _chunks{};
    mutable decltype(_chunks)::iterator _current_chunk{};

    friend class Wav;
};

} // namespace FileUtils