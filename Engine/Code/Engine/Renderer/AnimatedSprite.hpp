#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

class Material;
class Texture;
struct AnimatedSpriteDesc;

class AnimatedSprite {
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
    explicit AnimatedSprite(const XMLElement& elem) noexcept;
    explicit AnimatedSprite(const AnimatedSpriteDesc& desc) noexcept;
    AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const XMLElement& elem) noexcept;
    AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const IntVector2& startSpriteCoords) noexcept;
    ~AnimatedSprite() noexcept;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;
    [[nodiscard]] AABB2 GetCurrentTexCoords() const noexcept;
    [[nodiscard]] IntVector2 GetCurrentSpriteCoords() const noexcept;
    [[nodiscard]] const Texture* const GetTexture() const noexcept;
    [[nodiscard]] int GetNumSprites() const noexcept;
    [[nodiscard]] IntVector2 GetFrameDimensions() const noexcept;
    [[nodiscard]] int GetFrameCount() const noexcept;
    void TogglePause() noexcept;
    void Pause() noexcept;                                                  // Starts unpaused (playing) by default
    void Resume() noexcept;                                                 // Resume after pausing
    void Reset() noexcept;                                                  // Rewinds to time 0 and starts (re)playing
    [[nodiscard]] bool IsFinished() const noexcept;                         //{ return m_isFinished; }
    [[nodiscard]] bool IsPlaying() const noexcept;                          //{ return m_isPlaying; }
    [[nodiscard]] TimeUtils::FPSeconds GetDurationSeconds() const noexcept; //{ return m_durationSeconds; }
    [[nodiscard]] TimeUtils::FPSeconds GetSecondsElapsed() const noexcept;  //{ return m_elapsedSeconds; }
    [[nodiscard]] TimeUtils::FPSeconds GetSecondsRemaining() const noexcept;
    [[nodiscard]] float GetFractionElapsed() const noexcept;
    [[nodiscard]] float GetFractionRemaining() const noexcept;
    void SetSecondsElapsed(TimeUtils::FPSeconds secondsElapsed) noexcept; // Jump to specific time
    void SetFractionElapsed(float fractionElapsed) noexcept;              // e.g. 0.33f for one-third in
    void SetDuration(TimeUtils::FPSeconds durationSeconds) noexcept;
    void SetMaterial(Material* mat) noexcept;
    [[nodiscard]] Material* GetMaterial() const noexcept;

    const SpriteAnimMode& GetPlaybackMode() const noexcept;

protected:
private:
    AnimatedSprite(std::weak_ptr<SpriteSheet> spriteSheet, TimeUtils::FPSeconds durationSeconds, int startSpriteIndex, int frameLength, SpriteAnimMode playbackMode = SpriteAnimMode::Looping) noexcept;
    AnimatedSprite(std::weak_ptr<SpriteSheet> spriteSheet, TimeUtils::FPSeconds durationSeconds, const IntVector2& startSpriteCoords, int frameLength, SpriteAnimMode playbackMode = SpriteAnimMode::Looping) noexcept;

    void LoadFromXml(const XMLElement& elem) noexcept;
    [[nodiscard]] SpriteAnimMode GetAnimModeFromOptions(bool looping, bool backwards, bool ping_pong /*= false*/) noexcept;
    [[nodiscard]] int GetIndexFromCoords(const IntVector2& coords) noexcept;

    Material* _material = nullptr;
    std::weak_ptr<SpriteSheet> _sheet{};
    TimeUtils::FPSeconds _duration_seconds = TimeUtils::FPFrames{1};
    TimeUtils::FPSeconds _elapsed_seconds{0.0f};
    TimeUtils::FPSeconds _elapsed_frame_delta_seconds{0.0f};
    TimeUtils::FPSeconds _max_seconds_per_frame{0.0f};
    SpriteAnimMode _playback_mode = SpriteAnimMode::Looping;
    int _start_index{0};
    int _end_index{1};
    bool _is_playing = true;
};

struct AnimatedSpriteDesc {
    Material* material{};
    std::weak_ptr<SpriteSheet> spriteSheet{};
    TimeUtils::FPSeconds durationSeconds{TimeUtils::FPFrames{1.0f}};
    int startSpriteIndex{-1};
    IntVector2 startSpriteCoords{-1, -1};
    int frameLength{1};
    AnimatedSprite::SpriteAnimMode playbackMode = AnimatedSprite::SpriteAnimMode::Looping;
};
