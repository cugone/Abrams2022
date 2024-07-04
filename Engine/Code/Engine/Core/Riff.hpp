#pragma once

#include "Engine/Core/StringUtils.hpp"

#include <filesystem>
#include <optional>
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

namespace detail {
    class RiffHeader {
    public:
        RiffHeader() noexcept = default;
        RiffHeader(const RiffHeader& other) noexcept = default;
        RiffHeader(RiffHeader&& rother) noexcept = default;
        RiffHeader& operator=(const RiffHeader& rhs) noexcept = default;
        RiffHeader& operator=(RiffHeader&& rrhs) noexcept = default;
        ~RiffHeader() noexcept = default;

        char fourcc[4] = {0, 0, 0, 0};
        uint32_t length{0u};
    };
    class RiffSubChunk {
    public:
        RiffSubChunk() noexcept = default;
        RiffSubChunk(const RiffSubChunk& other) noexcept = delete;
        RiffSubChunk(RiffSubChunk&& rother) noexcept = default;
        RiffSubChunk& operator=(const RiffSubChunk& rhs) noexcept = delete;
        RiffSubChunk& operator=(RiffSubChunk&& rrhs) noexcept = default;
        ~RiffSubChunk() noexcept = default;

        char fourcc[4] = {0, 0, 0, 0};
        std::size_t subdata_length{0};
        std::unique_ptr<uint8_t[]> subdata{nullptr};
    };
    class RiffChunk {
    public:
        RiffChunk() noexcept = default;
        RiffChunk(const RiffChunk& other) noexcept = delete;
        RiffChunk(RiffChunk&& rother) noexcept = default;
        RiffChunk& operator=(const RiffChunk& rhs) noexcept = delete;
        RiffChunk& operator=(RiffChunk&& rrhs) noexcept = default;
        ~RiffChunk() noexcept = default;

        RiffHeader header;
        RiffSubChunk data;
    };
}

class Riff {
public:
    Riff() noexcept;
    Riff(const Riff& other) noexcept = delete;
    Riff(Riff&& rother) noexcept = default;
    Riff& operator=(const Riff& rhs) noexcept = delete;
    Riff& operator=(Riff&& rrhs) noexcept = default;
    ~Riff() noexcept;
    static constexpr const unsigned int RIFF_SUCCESS = 0;
    static constexpr const unsigned int RIFF_ERROR_NOT_A_RIFF = 1;
    static constexpr const unsigned int RIFF_ERROR_INVALID_RIFF = 2;
    static constexpr const unsigned int RIFF_ERROR_INVALID_ARGUMENT = 3;

    [[nodiscard]] detail::RiffChunk* GetNextChunk() const noexcept;
    [[nodiscard]] unsigned int Load(std::filesystem::path filename) noexcept;
    [[nodiscard]] unsigned int Load(std::vector<unsigned char>& data) noexcept;
    [[nodiscard]] static std::optional<detail::RiffChunk> ReadListChunk(std::stringstream& stream) noexcept;

protected:
private:
    [[nodiscard]] bool ParseDataIntoChunks(std::vector<uint8_t>& buffer) noexcept;

    std::vector<detail::RiffChunk> m_chunks;
    mutable decltype(m_chunks)::iterator m_current_chunk;

    friend class Wav;
};

} // namespace FileUtils