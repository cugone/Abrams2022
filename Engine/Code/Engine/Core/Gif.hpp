#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Texture.hpp"

#include <chrono>
#include <filesystem>
#include <memory>

namespace FileUtils {

struct GifDesc;

class Gif {
public:
    enum class PlayMode {
        PlayToEnd
        ,PlayToBeginning
        ,Loop
        ,Reverse
    };
    Gif() = default;
    Gif(const Gif& other) = default;
    Gif(Gif&& other) = default;
    Gif& operator=(const Gif& other) = default;
    Gif& operator=(Gif&& other) = default;
    ~Gif() noexcept = default;

    explicit Gif(const GifDesc& desc) noexcept;

    void SetPlayMode(const Gif::PlayMode& newMode) noexcept;
    void SetStartFrame(const std::size_t& newStartFrame) noexcept;
    void SetEndFrame(const std::size_t& newEndFrame) noexcept;
    void SetFrameRange(const std::size_t& newStartFrame, const std::size_t& newEndFrame) noexcept;

    bool Load(std::filesystem::path filepath) noexcept;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render(const Matrix4& transform = Matrix4::I) const noexcept;

    IntVector2 GetDimensions() const noexcept;
    void SetImage(std::filesystem::path newFilepath) noexcept;

protected:
private:
    std::vector<TimeUtils::FPMilliseconds> m_frameDelays{};
    Texture* m_texture{};
    TimeUtils::FPSeconds m_duration{};
    TimeUtils::FPSeconds m_frameDuration{};
    std::size_t m_currentFrame{0u};
    std::size_t m_startFrame{0u};
    std::size_t m_endFrame{static_cast<std::size_t>(-1)};
    std::size_t m_totalFrames{};
    Gif::PlayMode m_playMode{Gif::PlayMode::Loop};
};

struct GifDesc {
    std::filesystem::path filepath{};
    std::size_t startFrame{0u};
    std::size_t endFrame{static_cast<std::size_t>(-1)};
    Gif::PlayMode playMode{Gif::PlayMode::Loop};
};


} // namespace FileUtils

