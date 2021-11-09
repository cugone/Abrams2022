#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Texture2D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> spriteSheet,
                               TimeUtils::FPSeconds durationSeconds,
                               int startSpriteIndex,
                               int frameLength,
                               SpriteAnimMode playbackMode /*= SpriteAnimMode::LOOPING*/) noexcept
: _sheet(spriteSheet)
, _duration_seconds(durationSeconds)
, _playback_mode(playbackMode)
, _start_index(startSpriteIndex)
, _end_index(startSpriteIndex + frameLength) {
    bool has_frames = frameLength > 0;
    _max_seconds_per_frame = TimeUtils::FPSeconds{_duration_seconds / (has_frames ? static_cast<float>(_end_index - _start_index) : 1.0f)};
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> spriteSheet, TimeUtils::FPSeconds durationSeconds, const IntVector2& startSpriteCoords, int frameLength, SpriteAnimMode playbackMode /*= SpriteAnimMode::Looping*/) noexcept
: _sheet(spriteSheet)
, _duration_seconds(durationSeconds)
, _playback_mode(playbackMode)
, _start_index(startSpriteCoords.x + startSpriteCoords.y * _sheet.lock()->GetLayout().x)
, _end_index(_start_index + frameLength) {
    bool has_frames = frameLength > 0;
    _max_seconds_per_frame = TimeUtils::FPSeconds{_duration_seconds / (has_frames ? static_cast<float>(_end_index - _start_index) : 1.0f)};
}

AnimatedSprite::AnimatedSprite(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const XMLElement& elem) noexcept
: _sheet(sheet) {
    LoadFromXml(elem);
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const IntVector2& startSpriteCoords /* = IntVector2::ZERO*/) noexcept
: AnimatedSprite(sheet, TimeUtils::FPFrames{1}, startSpriteCoords, 0) {
    /* DO NOTHING */
}

AnimatedSprite::AnimatedSprite(const AnimatedSpriteDesc& desc) noexcept
: _material(desc.material)
, _sheet(desc.spriteSheet)
, _duration_seconds(desc.durationSeconds)
, _playback_mode(desc.playbackMode)
, _start_index(desc.startSpriteIndex > -1 ? desc.startSpriteIndex : (desc.startSpriteCoords.x > -1 && desc.startSpriteCoords.y > -1 ? (desc.startSpriteCoords.x + desc.startSpriteCoords.y * _sheet.lock()->GetLayout().x) : 0))
, _end_index(desc.startSpriteIndex + desc.frameLength) {
    bool has_frames = desc.frameLength > 0;
    _max_seconds_per_frame = TimeUtils::FPSeconds{_duration_seconds / (has_frames ? static_cast<float>(_end_index - _start_index) : 1.0f)};
}

AnimatedSprite::~AnimatedSprite() noexcept {
    _sheet.reset();
}

void AnimatedSprite::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    _elapsed_frame_delta_seconds += deltaSeconds;
    while(_elapsed_frame_delta_seconds >= _max_seconds_per_frame) {
        _elapsed_frame_delta_seconds -= _max_seconds_per_frame;
    }
    switch(_playback_mode) {
    case SpriteAnimMode::Looping:
        if(IsFinished()) {
            while(_elapsed_seconds >= _duration_seconds) {
                _elapsed_seconds -= _duration_seconds;
            }
        }
        break;
    case SpriteAnimMode::Looping_Reverse:
        if(IsFinished()) {
            _elapsed_seconds = _duration_seconds;
            deltaSeconds *= -1.0f;
        }
        break;
    case SpriteAnimMode::Play_To_Beginning:
        if(IsFinished()) {
            _is_playing = false;
            _elapsed_seconds = TimeUtils::FPSeconds{0.0f};
        }
        break;
    case SpriteAnimMode::Play_To_End:
        if(IsFinished()) {
            _is_playing = false;
            _elapsed_seconds = _duration_seconds;
        }
        break;
    case SpriteAnimMode::Ping_Pong:
        if(_elapsed_seconds < TimeUtils::FPSeconds{0.0f}) {
            deltaSeconds *= -1.0f;
            while(_elapsed_seconds < TimeUtils::FPSeconds{0.0f}) {
                _elapsed_seconds += _duration_seconds;
            }
        } else if(_elapsed_seconds >= _duration_seconds) {
            deltaSeconds *= -1.0f;
            while(_elapsed_seconds >= _duration_seconds) {
                _elapsed_seconds -= _duration_seconds;
            }
        }
        break;
    default:
        break;
    }
    _elapsed_seconds += deltaSeconds;
}

AABB2 AnimatedSprite::GetCurrentTexCoords() const noexcept {
    const auto&& [x, y] = GetCurrentSpriteCoords();
    if(!_sheet.expired()) {
        return _sheet.lock()->GetTexCoordsFromSpriteCoords(x, y);
    }
    return {};
}

IntVector2 AnimatedSprite::GetCurrentSpriteCoords() const noexcept {
    int length = _end_index - _start_index;
    const auto framesPerSecond = TimeUtils::FPSeconds{1.0f} / _max_seconds_per_frame;
    auto frameIndex = static_cast<int>(_elapsed_seconds.count() * framesPerSecond);
    switch(_playback_mode) {
    case SpriteAnimMode::Play_To_End:
        if(frameIndex >= length) {
            frameIndex = _end_index - 1;
        }
        break;
    case SpriteAnimMode::Play_To_Beginning:
        if(frameIndex < 0) {
            frameIndex = 0;
        }
        break;
    case SpriteAnimMode::Looping: /* FALLTHROUGH */
    case SpriteAnimMode::Looping_Reverse:
        if(frameIndex >= length) {
            frameIndex = 0;
        }
        if(frameIndex < 0) {
            frameIndex = _end_index - 1;
        }
        break;
    case SpriteAnimMode::Ping_Pong:
        if(frameIndex >= length) {
            frameIndex = _end_index - 1;
        }
        if(frameIndex < 0) {
            frameIndex = 0;
        }
        break;
    default:
        break;
    }
    const auto spriteIndex = _start_index + frameIndex;
    const auto tileWidth = [this]() {
        if(!_sheet.expired()) {
            return _sheet.lock()->GetLayout().x;
        }
        return 1;
    }();
    const auto x = spriteIndex % tileWidth;
    const auto y = spriteIndex / tileWidth;
    return IntVector2{x, y};
}

const Texture* const AnimatedSprite::GetTexture() const noexcept {
    if(!_sheet.expired()) {
        return _sheet.lock()->GetTexture();
    }
    return nullptr;
}

int AnimatedSprite::GetNumSprites() const noexcept {
    if(!_sheet.expired()) {
        return _sheet.lock()->GetNumSprites();
    }
    return 0;
}

IntVector2 AnimatedSprite::GetFrameDimensions() const noexcept {
    if(!_sheet.expired()) {
        return _sheet.lock()->GetFrameDimensions();
    }
    return {};
}

int AnimatedSprite::GetFrameCount() const noexcept {
    return _end_index - _start_index;
}

void AnimatedSprite::TogglePause() noexcept {
    _is_playing = !_is_playing;
}

void AnimatedSprite::Pause() noexcept {
    _is_playing = false;
}

void AnimatedSprite::Resume() noexcept {
    _is_playing = true;
}

void AnimatedSprite::Reset() noexcept {
    _elapsed_seconds = TimeUtils::FPSeconds{0.0f};
}

bool AnimatedSprite::IsFinished() const noexcept {
    if(!_is_playing) {
        return false;
    }
    switch(_playback_mode) {
    case SpriteAnimMode::Looping: /* FALL THROUGH */
    case SpriteAnimMode::Play_To_End:
        return !(_elapsed_seconds < _duration_seconds);
    case SpriteAnimMode::Looping_Reverse: /* FALL THROUGH */
    case SpriteAnimMode::Play_To_Beginning:
        return _elapsed_seconds < TimeUtils::FPSeconds{0.0f};
    case SpriteAnimMode::Ping_Pong:
        return false;
    default:
        return !(_elapsed_seconds < _duration_seconds);
    }
}

bool AnimatedSprite::IsPlaying() const noexcept {
    return _is_playing;
}

TimeUtils::FPSeconds AnimatedSprite::GetDurationSeconds() const noexcept {
    return _duration_seconds;
}

TimeUtils::FPSeconds AnimatedSprite::GetSecondsElapsed() const noexcept {
    return _elapsed_seconds;
}

TimeUtils::FPSeconds AnimatedSprite::GetSecondsRemaining() const noexcept {
    return _duration_seconds - _elapsed_seconds;
}

float AnimatedSprite::GetFractionElapsed() const noexcept {
    return _elapsed_seconds / _duration_seconds;
}

float AnimatedSprite::GetFractionRemaining() const noexcept {
    return (_duration_seconds - _elapsed_seconds) / _duration_seconds;
}

void AnimatedSprite::SetSecondsElapsed(TimeUtils::FPSeconds secondsElapsed) noexcept {
    _elapsed_seconds = secondsElapsed;
}

void AnimatedSprite::SetFractionElapsed(float fractionElapsed) noexcept {
    _elapsed_seconds = _duration_seconds * fractionElapsed;
}

void AnimatedSprite::SetDuration(TimeUtils::FPSeconds durationSeconds) noexcept {
    _duration_seconds = durationSeconds;
    bool has_frames = _end_index - _start_index > 0;
    _max_seconds_per_frame = TimeUtils::FPSeconds{_duration_seconds / (has_frames ? static_cast<float>(_end_index - _start_index) : 1.0f)};
}

void AnimatedSprite::SetMaterial(Material* mat) noexcept {
    _material = mat;
}

Material* AnimatedSprite::GetMaterial() const noexcept {
    return _material;
}

const AnimatedSprite::SpriteAnimMode& AnimatedSprite::GetPlaybackMode() const noexcept {
    return _playback_mode;
}

AnimatedSprite::SpriteAnimMode AnimatedSprite::GetAnimModeFromOptions(bool looping, bool backwards, bool ping_pong /*= false*/) noexcept {
    if(ping_pong) {
        return SpriteAnimMode::Ping_Pong;
    }

    if(looping) {
        if(backwards) {
            return SpriteAnimMode::Looping_Reverse;
        }
        return SpriteAnimMode::Looping;
    }
    if(backwards) {
        return SpriteAnimMode::Play_To_Beginning;
    }
    return SpriteAnimMode::Play_To_End;
}

int AnimatedSprite::GetIndexFromCoords(const IntVector2& coords) noexcept {
    if(!_sheet.expired()) {
        const auto& layout = _sheet.lock()->GetLayout();
        return coords.x + coords.y * layout.x;
    }
    return 0;
}

void AnimatedSprite::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "animation", "animationset", "", "spritesheet", "name");
    if(const auto* xml_sheet = elem.FirstChildElement("spritesheet")) {
        DataUtils::ValidateXmlElement(*xml_sheet, "spritesheet", "", "src,dimensions");
        _sheet = ServiceLocator::get<IRendererService>().CreateSpriteSheet(*xml_sheet);
    }

    const auto* xml_animset = elem.FirstChildElement("animationset");
    DataUtils::ValidateXmlElement(*xml_animset, "animationset", "", "startindex,framelength,duration", "", "loop,reverse,pingpong");

    _start_index = DataUtils::ParseXmlAttribute(*xml_animset, "startindex", -1);
    if(_start_index == -1) {
        const auto start_index_coords = DataUtils::ParseXmlAttribute(*xml_animset, "startindex", IntVector2::Zero);
        _start_index = GetIndexFromCoords(start_index_coords);
    }

    const auto frameLength = DataUtils::ParseXmlAttribute(*xml_animset, "framelength", 0);
    _end_index = _start_index + frameLength;

    TimeUtils::FPSeconds min_duration = TimeUtils::FPFrames{1};
    _duration_seconds = TimeUtils::FPSeconds{DataUtils::ParseXmlAttribute(*xml_animset, "duration", 0.0f)};
    if(_duration_seconds < min_duration) {
        _duration_seconds = min_duration;
    }

    const auto is_looping = DataUtils::ParseXmlAttribute(*xml_animset, "loop", false);
    const auto is_reverse = DataUtils::ParseXmlAttribute(*xml_animset, "reverse", false);
    const auto is_pingpong = DataUtils::ParseXmlAttribute(*xml_animset, "pingpong", false);
    _playback_mode = GetAnimModeFromOptions(is_looping, is_reverse, is_pingpong);

    const auto has_frames = frameLength > 0;
    _max_seconds_per_frame = TimeUtils::FPSeconds{_duration_seconds / (has_frames ? static_cast<float>(_end_index - _start_index) : 1.0f)};
}
