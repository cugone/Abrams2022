#pragma once

#include "Engine/Core/Riff.hpp"
#include "Engine/Core/StringUtils.hpp"

#include <filesystem>
#include <string>

namespace FileUtils {

namespace WavChunkID {
constexpr const uint32_t FMT = StringUtils::FourCC("fmt ");
constexpr const uint32_t FACT = StringUtils::FourCC("fact");
constexpr const uint32_t DATA = StringUtils::FourCC("data");
constexpr const bool IsValid(const char* id) noexcept;
} // namespace WavChunkID

class Wav {
public:
    struct WavHeader {
        char fourcc[4]{};
        uint32_t length{};
    };

    struct WavFormatChunk {
        uint16_t formatId{};
        uint16_t channelCount{};
        uint32_t samplesPerSecond{};
        uint32_t bytesPerSecond{}; //samplesPerSecond * channelCount * bitsPerSample / 8
        uint16_t dataBlockSize{};  //channelCount * bitsPerSample / 8
        uint16_t bitsPerSample{};
        uint16_t extensionSize{};
        uint16_t validBitsPerSample{};
        uint32_t speakerPositionMask{};
        uint32_t subFormat[4]{};
    };

    struct WavFactChunk {
        uint32_t samplesPerChannel{};
    };

    struct WavDataChunk {
        uint32_t length{};
        std::unique_ptr<uint8_t[]> data;
    };

    static constexpr const unsigned int WAV_SUCCESS = 0;
    static constexpr const unsigned int WAV_ERROR_NOT_A_WAV = 1;
    static constexpr const unsigned int WAV_ERROR_BAD_FILE = 2;

    [[nodiscard]] unsigned int Load(std::filesystem::path filepath) noexcept;
    [[nodiscard]] unsigned char* GetFormatAsBuffer() noexcept;
    [[nodiscard]] unsigned char* GetDataBuffer() const noexcept;
    [[nodiscard]] const WavFormatChunk& GetFormatChunk() const noexcept;
    [[nodiscard]] const WavDataChunk& GetDataChunk() const noexcept;
    [[nodiscard]] uint32_t GetDataBufferSize() const noexcept;

protected:
private:
    WavFormatChunk _fmt{};
    WavFactChunk _fact{};
    WavDataChunk _data{};
};

} // namespace FileUtils