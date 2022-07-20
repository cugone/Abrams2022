#pragma once

#include "Engine/Core/StringUtils.hpp"

#include <filesystem>

namespace FileUtils {

namespace WavChunkID {
constexpr const uint32_t FMT = StringUtils::FourCC("fmt ");
constexpr const uint32_t FACT = StringUtils::FourCC("fact");
constexpr const uint32_t DATA = StringUtils::FourCC("data");
constexpr const bool IsValid(const char* id) noexcept;
} // namespace WavChunkID

namespace detail {
struct WavHeader {
    char fourcc[4]{0, 0, 0, 0};
    uint32_t length{0u};
};

struct WavFormatChunk {
    uint16_t formatId{0u};
    uint16_t channelCount{0u};
    uint32_t samplesPerSecond{0u};
    uint32_t bytesPerSecond{0u}; //samplesPerSecond * channelCount * bitsPerSample / 8
    uint16_t dataBlockSize{0u};  //channelCount * bitsPerSample / 8
    uint16_t bitsPerSample{0u};
    uint16_t extensionSize{0u};
    uint16_t validBitsPerSample{0u};
    uint32_t speakerPositionMask{0u};
    uint32_t subFormat[4]{0u, 0u, 0u, 0u};
};

struct WavFactChunk {
    uint32_t samplesPerChannel{0u};
};

struct WavDataChunk {
    uint32_t length{0u};
    std::unique_ptr<uint8_t[]> data{nullptr};
};

}

class Wav {
public:
    Wav() = default;
    Wav(const Wav& other) = delete;
    Wav(Wav&& rother) = default;
    Wav& operator=(const Wav& rhs) = delete;
    Wav& operator=(Wav&& rhs) = default;
    ~Wav() = default;

    static constexpr const unsigned int WAV_SUCCESS = 0;
    static constexpr const unsigned int WAV_ERROR_NOT_A_WAV = 1;
    static constexpr const unsigned int WAV_ERROR_BAD_FILE = 2;

    [[nodiscard]] unsigned int Load(std::filesystem::path filepath) noexcept;
    [[nodiscard]] unsigned char* GetFormatAsBuffer() noexcept;
    [[nodiscard]] unsigned char* GetDataBuffer() const noexcept;
    [[nodiscard]] const detail::WavFormatChunk& GetFormatChunk() const noexcept;
    [[nodiscard]] const detail::WavDataChunk& GetDataChunk() const noexcept;
    [[nodiscard]] uint32_t GetDataBufferSize() const noexcept;

protected:
private:
    detail::WavFormatChunk m_fmt;
    detail::WavFactChunk m_fact;
    detail::WavDataChunk m_data;
};

} // namespace FileUtils