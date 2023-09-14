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
: m_sheet(spriteSheet)
, m_duration_seconds(durationSeconds)
, m_playback_mode(playbackMode)
, m_start_index(startSpriteIndex)
, m_end_index(startSpriteIndex + frameLength) {
    bool has_frames = frameLength > 0;
    m_max_seconds_per_frame = TimeUtils::FPSeconds{m_duration_seconds / (has_frames ? static_cast<float>(m_end_index - m_start_index) : 1.0f)};
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> spriteSheet, TimeUtils::FPSeconds durationSeconds, const IntVector2& startSpriteCoords, int frameLength, SpriteAnimMode playbackMode /*= SpriteAnimMode::Looping*/) noexcept
: m_sheet(spriteSheet)
, m_duration_seconds(durationSeconds)
, m_playback_mode(playbackMode)
, m_start_index(startSpriteCoords.x + startSpriteCoords.y * m_sheet.lock()->GetLayout().x)
, m_end_index(m_start_index + frameLength) {
    bool has_frames = frameLength > 0;
    m_max_seconds_per_frame = TimeUtils::FPSeconds{m_duration_seconds / (has_frames ? static_cast<float>(m_end_index - m_start_index) : 1.0f)};
}

AnimatedSprite::AnimatedSprite(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const XMLElement& elem) noexcept
: m_sheet(sheet) {
    LoadFromXml(elem);
}

AnimatedSprite::AnimatedSprite(std::weak_ptr<SpriteSheet> sheet, const IntVector2& startSpriteCoords /* = IntVector2::ZERO*/) noexcept
: AnimatedSprite(sheet, TimeUtils::FPFrames{1}, startSpriteCoords, 0) {
    /* DO NOTHING */
}

AnimatedSprite::AnimatedSprite(const AnimatedSpriteDesc& desc) noexcept
: m_material(desc.material)
, m_sheet(desc.spriteSheet)
, m_duration_seconds(desc.durationSeconds)
, m_playback_mode(desc.playbackMode)
, m_start_index(desc.startSpriteIndex > -1 ? desc.startSpriteIndex : (desc.startSpriteCoords.x > -1 && desc.startSpriteCoords.y > -1 ? (desc.startSpriteCoords.x + desc.startSpriteCoords.y * m_sheet.lock()->GetLayout().x) : 0))
, m_end_index(desc.startSpriteIndex + desc.frameLength) {
    bool has_frames = desc.frameLength > 0;
    m_max_seconds_per_frame = TimeUtils::FPSeconds{m_duration_seconds / (has_frames ? static_cast<float>(m_end_index - m_start_index) : 1.0f)};
}

AnimatedSprite::~AnimatedSprite() noexcept {
    m_sheet.reset();
}

void AnimatedSprite::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_elapsed_frame_delta_seconds += deltaSeconds;
    while(m_elapsed_frame_delta_seconds >= m_max_seconds_per_frame) {
        m_elapsed_frame_delta_seconds -= m_max_seconds_per_frame;
    }
    switch(m_playback_mode) {
    case SpriteAnimMode::Looping:
        if(IsFinished()) {
            while(m_elapsed_seconds >= m_duration_seconds) {
                m_elapsed_seconds -= m_duration_seconds;
            }
        }
        break;
    case SpriteAnimMode::Looping_Reverse:
        if(IsFinished()) {
            m_elapsed_seconds = m_duration_seconds;
            deltaSeconds *= -1.0f;
        }
        break;
    case SpriteAnimMode::Play_To_Beginning:
        if(IsFinished()) {
            m_is_playing = false;
            m_elapsed_seconds = TimeUtils::FPSeconds{0.0f};
        }
        break;
    case SpriteAnimMode::Play_To_End:
        if(IsFinished()) {
            m_is_playing = false;
            m_elapsed_seconds = m_duration_seconds;
        }
        break;
    case SpriteAnimMode::Ping_Pong:
        if(m_elapsed_seconds < TimeUtils::FPSeconds{0.0f}) {
            deltaSeconds *= -1.0f;
            while(m_elapsed_seconds < TimeUtils::FPSeconds{0.0f}) {
                m_elapsed_seconds += m_duration_seconds;
            }
        } else if(m_elapsed_seconds >= m_duration_seconds) {
            deltaSeconds *= -1.0f;
            while(m_elapsed_seconds >= m_duration_seconds) {
                m_elapsed_seconds -= m_duration_seconds;
            }
        }
        break;
    default:
        break;
    }
    m_elapsed_seconds += deltaSeconds;
}

AABB2 AnimatedSprite::GetCurrentTexCoords() const noexcept {
    const auto&& [x, y] = GetCurrentSpriteCoords();
    if(!m_sheet.expired()) {
        return m_sheet.lock()->GetTexCoordsFromSpriteCoords(x, y);
    }
    return {};
}

IntVector2 AnimatedSprite::GetCurrentSpriteCoords() const noexcept {
    const auto frameIndex = [this]() {
        const auto framesPerSecond = TimeUtils::FPSeconds{1.0f} / m_max_seconds_per_frame;
        const auto length = m_end_index - m_start_index;
        const auto first = 0;
        const auto last = m_end_index - 1;
        const auto result = static_cast<int>(m_elapsed_seconds.count() * framesPerSecond);
        switch(m_playback_mode) {
        case SpriteAnimMode::Play_To_End:
            if(result >= length) {
                return last;
            }
            break;
        case SpriteAnimMode::Play_To_Beginning:
            if(result < first) {
                return first;
            }
            break;
        case SpriteAnimMode::Looping: /* FALLTHROUGH */
        case SpriteAnimMode::Looping_Reverse:
            if(result >= length) {
                return first;
            }
            if(result < first) {
                return last;
            }
            break;
        case SpriteAnimMode::Ping_Pong:
            if(result >= length) {
                return last;
            }
            if(result < first) {
                return first;
            }
            break;
        }
        return result;
    }(); //IIIL
    const auto spriteIndex = m_start_index + frameIndex;
    const auto tileWidth = [this]() {
        if(!m_sheet.expired()) {
            return m_sheet.lock()->GetLayout().x;
        }
        return 1;
    }();
    const auto x = spriteIndex % tileWidth;
    const auto y = spriteIndex / tileWidth;
    return IntVector2{x, y};
}

const Texture* const AnimatedSprite::GetTexture() const noexcept {
    if(!m_sheet.expired()) {
        return m_sheet.lock()->GetTexture();
    }
    return nullptr;
}

int AnimatedSprite::GetNumSprites() const noexcept {
    if(!m_sheet.expired()) {
        return m_sheet.lock()->GetNumSprites();
    }
    return 0;
}

IntVector2 AnimatedSprite::GetFrameDimensions() const noexcept {
    if(!m_sheet.expired()) {
        return m_sheet.lock()->GetFrameDimensions();
    }
    return {};
}

int AnimatedSprite::GetFrameCount() const noexcept {
    return m_end_index - m_start_index;
}

void AnimatedSprite::TogglePause() noexcept {
    m_is_playing = !m_is_playing;
}

void AnimatedSprite::Pause() noexcept {
    m_is_playing = false;
}

void AnimatedSprite::Resume() noexcept {
    m_is_playing = true;
}

void AnimatedSprite::Reset() noexcept {
    m_elapsed_seconds = TimeUtils::FPSeconds{0.0f};
}

bool AnimatedSprite::IsFinished() const noexcept {
    if(!m_is_playing) {
        return false;
    }
    switch(m_playback_mode) {
    case SpriteAnimMode::Looping: /* FALL THROUGH */
    case SpriteAnimMode::Play_To_End:
        return !(m_elapsed_seconds < m_duration_seconds);
    case SpriteAnimMode::Looping_Reverse: /* FALL THROUGH */
    case SpriteAnimMode::Play_To_Beginning:
        return m_elapsed_seconds < TimeUtils::FPSeconds{0.0f};
    case SpriteAnimMode::Ping_Pong:
        return false;
    default:
        return !(m_elapsed_seconds < m_duration_seconds);
    }
}

bool AnimatedSprite::IsPlaying() const noexcept {
    return m_is_playing;
}

TimeUtils::FPSeconds AnimatedSprite::GetDurationSeconds() const noexcept {
    return m_duration_seconds;
}

TimeUtils::FPSeconds AnimatedSprite::GetSecondsElapsed() const noexcept {
    return m_elapsed_seconds;
}

TimeUtils::FPSeconds AnimatedSprite::GetSecondsRemaining() const noexcept {
    return m_duration_seconds - m_elapsed_seconds;
}

float AnimatedSprite::GetFractionElapsed() const noexcept {
    return m_elapsed_seconds / m_duration_seconds;
}

float AnimatedSprite::GetFractionRemaining() const noexcept {
    return (m_duration_seconds - m_elapsed_seconds) / m_duration_seconds;
}

void AnimatedSprite::SetSecondsElapsed(TimeUtils::FPSeconds secondsElapsed) noexcept {
    m_elapsed_seconds = secondsElapsed;
}

void AnimatedSprite::SetFractionElapsed(float fractionElapsed) noexcept {
    m_elapsed_seconds = m_duration_seconds * fractionElapsed;
}

void AnimatedSprite::SetDuration(TimeUtils::FPSeconds durationSeconds) noexcept {
    m_duration_seconds = durationSeconds;
    bool has_frames = m_end_index - m_start_index > 0;
    m_max_seconds_per_frame = TimeUtils::FPSeconds{m_duration_seconds / (has_frames ? static_cast<float>(m_end_index - m_start_index) : 1.0f)};
}

void AnimatedSprite::SetMaterial(Material* mat) noexcept {
    m_material = mat;
}

Material* AnimatedSprite::GetMaterial() const noexcept {
    return m_material;
}

const AnimatedSprite::SpriteAnimMode& AnimatedSprite::GetPlaybackMode() const noexcept {
    return m_playback_mode;
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
    if(!m_sheet.expired()) {
        const auto& layout = m_sheet.lock()->GetLayout();
        return coords.x + coords.y * layout.x;
    }
    return 0;
}

void AnimatedSprite::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "animation", "animationset", "", "spritesheet", "name");
    if(const auto* xml_sheet = elem.FirstChildElement("spritesheet")) {
        DataUtils::ValidateXmlElement(*xml_sheet, "spritesheet", "", "src,dimensions");
        m_sheet = ServiceLocator::get<IRendererService>()->CreateSpriteSheet(*xml_sheet);
    }

    const auto* xml_animset = elem.FirstChildElement("animationset");
    DataUtils::ValidateXmlElement(*xml_animset, "animationset", "", "startindex,framelength,duration", "", "loop,reverse,pingpong");

    m_start_index = DataUtils::ParseXmlAttribute(*xml_animset, "startindex", -1);
    if(m_start_index == -1) {
        const auto start_index_coords = DataUtils::ParseXmlAttribute(*xml_animset, "startindex", IntVector2::Zero);
        m_start_index = GetIndexFromCoords(start_index_coords);
    }

    const auto frameLength = DataUtils::ParseXmlAttribute(*xml_animset, "framelength", 0);
    m_end_index = m_start_index + frameLength;

    TimeUtils::FPSeconds min_duration = TimeUtils::FPFrames{1};
    m_duration_seconds = TimeUtils::FPSeconds{DataUtils::ParseXmlAttribute(*xml_animset, "duration", 0.0f)};
    if(m_duration_seconds < min_duration) {
        m_duration_seconds = min_duration;
    }

    const auto is_looping = DataUtils::ParseXmlAttribute(*xml_animset, "loop", false);
    const auto is_reverse = DataUtils::ParseXmlAttribute(*xml_animset, "reverse", false);
    const auto is_pingpong = DataUtils::ParseXmlAttribute(*xml_animset, "pingpong", false);
    m_playback_mode = GetAnimModeFromOptions(is_looping, is_reverse, is_pingpong);

    const auto has_frames = frameLength > 0;
    m_max_seconds_per_frame = TimeUtils::FPSeconds{m_duration_seconds / (has_frames ? static_cast<float>(m_end_index - m_start_index) : 1.0f)};
}
