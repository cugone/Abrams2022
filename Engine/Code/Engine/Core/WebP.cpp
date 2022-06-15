#include "Engine/Core/WebP.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Vertex3D.hpp"

#include <algorithm>
#include <execution>
#include <numeric>

namespace FileUtils {

WebP::WebP(const std::filesystem::path& path) noexcept {
    LoadWebPData(path);
}

WebP::WebP(const XMLElement& elem) noexcept {
    LoadFromXml(elem);
}

void WebP::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {

    static TimeUtils::FPSeconds frameDuration{};
    static TimeUtils::FPSeconds elapsedDuration{};
    if(frameDuration >= m_frameDurations[m_currentFrame]) {
        ++m_currentFrame;
        frameDuration = TimeUtils::FPSeconds::zero();
        if(m_currentFrame < m_beginFrame) {
            m_currentFrame = m_beginFrame;
        }
        if(m_currentFrame >= m_endFrame) {
            m_currentFrame = m_beginFrame;
        }
        if(m_currentFrame >= m_maxFrames) {
            m_currentFrame = m_maxFrames - 1;
        }
    }
    frameDuration += deltaSeconds;
    elapsedDuration += deltaSeconds;
    if(elapsedDuration >= m_totalDuration) {
        elapsedDuration = TimeUtils::FPSeconds::zero();
        m_currentFrame = m_beginFrame;
    }

    //TODO: Implement Animation Modes
    //static TimeUtils::FPSeconds frameDuration{};
    //static bool advance_frame = false;
    //if(frameDuration >= m_frameDurations[m_currentFrame]) {
    //    frameDuration = TimeUtils::FPSeconds::zero();
    //    advance_frame = true;
    //}
    //switch(m_playback_mode) {
    //case SpriteAnimMode::Play_To_End:
    //    if(advance_frame && ++m_currentFrame >= m_endFrame) {
    //        m_currentFrame = m_endFrame - 1;
    //        advance_frame = false;
    //    }
    //    break;
    //case SpriteAnimMode::Play_To_Beginning:
    //    if(advance_frame && --m_currentFrame <= m_beginFrame) {
    //        m_currentFrame = m_beginFrame;
    //        advance_frame = false;
    //    }
    //    break;
    //case SpriteAnimMode::Looping:
    //    if(advance_frame && ++m_currentFrame >= m_endFrame) {
    //        m_currentFrame = m_beginFrame;
    //        advance_frame = false;
    //    }
    //    break;
    //case SpriteAnimMode::Looping_Reverse:
    //    if(advance_frame && --m_currentFrame <= m_beginFrame) {
    //        m_currentFrame = m_endFrame - 1;
    //        advance_frame = false;
    //    }
    //    break;
    //case SpriteAnimMode::Ping_Pong:
    //{
    //    static bool play_forwards = true;
    //    if(advance_frame) {
    //        if(play_forwards) {
    //            if(++m_currentFrame >= m_endFrame) {
    //                play_forwards = false;
    //                m_currentFrame = m_endFrame - 1;
    //            }
    //        } else {
    //            if(--m_currentFrame <= m_beginFrame) {
    //                play_forwards = true;
    //                m_currentFrame = m_beginFrame;
    //            }
    //        }
    //    }
    //}
    //break;
    //default:
    //    /* DO NOTHING */
    //    break;
    //}
    //frameDuration += deltaSeconds;
}

void WebP::Render() const noexcept {
    //TODO: Set up using webp shader.

    auto* r = ServiceLocator::get<IRendererService, NullRendererService>();
    r->SetMaterial(r->GetMaterial("__2D"));
    const auto S = Matrix4::CreateScaleMatrix(Vector2{1.0f, -1.0f});
    //const auto S = Matrix4::I;
    const auto R = Matrix4::I;
    const auto T = Matrix4::I;
    const auto M = Matrix4::MakeSRT(S, R, T);
    r->SetModelMatrix(M);
    r->SetTexture(m_frames[m_currentFrame].get());
    r->DrawQuad2D();
    r->SetTexture(nullptr);
}

const Texture* WebP::GetTexture(std::size_t index) const noexcept {
    return m_frames[index].get();
}

std::size_t WebP::GetFrameCount() const noexcept {
    return m_frameCount;
}

std::size_t WebP::GetFrameCount() noexcept {
    return m_frameCount;
}

void WebP::SetBeginFrame(std::size_t newBeginFrame) noexcept {
    if(newBeginFrame < m_endFrame) {
        m_beginFrame = newBeginFrame;
    }
}

void WebP::SetEndFrame(std::size_t newEndFrame) noexcept {
    if(m_beginFrame < newEndFrame) {
        m_endFrame = newEndFrame;
    }
}

void WebP::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "animation", "animationset", "", "", "name");

    const auto* xml_animset = elem.FirstChildElement("animationset");
    DataUtils::ValidateXmlElement(*xml_animset, "animationset", "", "src", "", "beginframe,endframe,loop,reverse,pingpong");

    if(const auto src = DataUtils::ParseXmlAttribute(*xml_animset, "src", std::string{}); !src.empty()) {
        LoadWebPData(src);
    }

    m_beginFrame = DataUtils::ParseXmlAttribute(*xml_animset, "beginframe", 0);
    m_endFrame = DataUtils::ParseXmlAttribute(*xml_animset, "endframe", m_frameCount);

    const auto is_looping = DataUtils::ParseXmlAttribute(*xml_animset, "loop", false);
    const auto is_reverse = DataUtils::ParseXmlAttribute(*xml_animset, "reverse", false);
    const auto is_pingpong = DataUtils::ParseXmlAttribute(*xml_animset, "pingpong", false);
    m_playback_mode = GetAnimModeFromOptions(is_looping, is_reverse, is_pingpong);

}

//TODO: Async?
void WebP::LoadWebPData(const std::filesystem::path& src) noexcept {
    if(auto buffer = FileUtils::ReadBinaryBufferFromFile(src); buffer.has_value()) {

        WebPData webp_data{};
        webp_data.bytes = buffer->data();
        webp_data.size = buffer->size();

        WebPAnimDecoderOptions dec_options{};
        WebPAnimDecoderOptionsInit(&dec_options);
        dec_options.color_mode = MODE_RGBA;
        if(WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options); dec != nullptr) {
            WebPAnimInfo anim_info{};
            WebPAnimDecoderGetInfo(dec, &anim_info);
            m_frames.resize(anim_info.frame_count);
            m_frameDurations.resize(anim_info.frame_count);
            m_frameCount = anim_info.frame_count;
            m_maxFrames = m_frameCount;
            m_beginFrame = m_maxFrames - m_maxFrames;
            m_endFrame = m_maxFrames;
            m_currentFrame = m_beginFrame;
            {
                auto* r = ServiceLocator::get<IRendererService, NullRendererService>();
                for(int i = 0; WebPAnimDecoderHasMoreFrames(dec); ++i) {
                    uint8_t* buf{};
                    static int cur_timestamp = 0;
                    static int prev_timestamp = 0;
                    if(WebPAnimDecoderGetNext(dec, &buf, &cur_timestamp)) {
                        m_frameDurations[i] = std::chrono::milliseconds{cur_timestamp - prev_timestamp};
                        prev_timestamp = cur_timestamp;
                        m_frames[i] = r->Create2DTextureFromMemory(buf, anim_info.canvas_width, anim_info.canvas_height);
                    }
                }
                WebPAnimDecoderDelete(dec);
                dec = nullptr;
                buffer->clear();
                buffer->shrink_to_fit();
            }
            m_totalDuration = std::reduce(std::execution::par_unseq, std::cbegin(m_frameDurations), std::cend(m_frameDurations), TimeUtils::FPSeconds::zero());
        }
    }
}

WebP::SpriteAnimMode WebP::GetAnimModeFromOptions(bool looping, bool backwards, bool ping_pong /*= false*/) noexcept {
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

}
