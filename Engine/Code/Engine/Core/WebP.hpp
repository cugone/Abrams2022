#pragma once

#include <filesystem>

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Texture.hpp"

#include <Thirdparty/TinyXML2/tinyxml2.h>
#include <Thirdparty/webp/demux.h>
#include <Thirdparty/webp/decode.h>
#include <Thirdparty/webp/types.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace FileUtils {

class WebP {
public:
    // clang-format off
    enum class SpriteAnimMode : int {
        Play_To_End       // Play from time=0 to durationSeconds, then finish
        , Play_To_Beginning // Play from time=durationSeconds to 0, then finish
        , Looping           // Play from time=0 to end then repeat (never finish)
        , Looping_Reverse   // Play from time=durationSeconds then repeat (never finish)
        , Ping_Pong         // Play forwards, backwards, forwards, backwards...
        , Max
    };
    // clang-format on

    WebP() = delete;
    //Direct .webp file
    explicit WebP(const std::filesystem::path& path) noexcept;
    //Animation element
    explicit WebP(const XMLElement& elem) noexcept;
    WebP(const WebP& other) noexcept = default;
    WebP(WebP&& rother) noexcept = default;
    WebP& operator=(const WebP& rhs) noexcept = default;
    WebP& operator=(WebP&& rrhs) noexcept = default;
    ~WebP() noexcept = default;

    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render() const noexcept;
    //const Texture* GetTexture(std::size_t index) const noexcept;

    std::size_t GetFrameCount() const noexcept;
    std::size_t GetFrameCount() noexcept;

    void SetBeginFrame(std::size_t newBeginFrame) noexcept;
    void SetEndFrame(std::size_t newEndFrame) noexcept;

protected:
private:
    void LoadFromXml(const XMLElement& elem) noexcept;
    void LoadWebPData(const std::filesystem::path& src) noexcept;

    SpriteAnimMode GetAnimModeFromOptions(bool looping, bool backwards, bool ping_pong /*= false*/) noexcept;

    std::unique_ptr<Texture> m_frames{};
    std::size_t m_currentFrame{0u};
    std::size_t m_beginFrame{0u};
    std::size_t m_endFrame{0u};
    std::size_t m_maxFrames{0u};
    SpriteAnimMode m_playback_mode{SpriteAnimMode::Play_To_End};
    uint32_t m_width{0u};
    uint32_t m_height{0u};
    uint32_t m_flags{0u};
    uint32_t m_frameCount{0u};
    uint32_t m_loopCount{0u};
    uint32_t m_maxLoopCount{0u};
    std::vector<TimeUtils::FPMilliseconds> m_frameDurations{};
    TimeUtils::FPSeconds m_totalDuration{};
    TimeUtils::FPSeconds m_frameDuration{};
    TimeUtils::FPSeconds m_elapsedDuration{};
    Rgba m_bgColor{Rgba::Black};
    bool m_isAnimated{false};

};

} // namespace FileUtils
