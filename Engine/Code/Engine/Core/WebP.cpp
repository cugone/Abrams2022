#include "Engine/Core/WebP.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Rgba.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Vertex3D.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

#include <algorithm>
#include <execution>
#include <numeric>
#include <string>

namespace FileUtils {

WebP::WebP(const std::filesystem::path& path) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    LoadWebPData(path);
}

WebP::WebP(const XMLElement& elem) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    LoadFromXml(elem);
}

void WebP::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(m_frameDuration >= m_frameDurations[m_currentFrame]) {
        ++m_currentFrame;
        m_frameDuration = TimeUtils::FPSeconds::zero();
        if(m_currentFrame < m_beginFrame) {
            m_currentFrame = m_beginFrame;
        }
        if(m_currentFrame >= m_endFrame) {
            m_currentFrame = m_beginFrame;
            if(m_maxLoopCount > 0u) {
                if(m_loopCount == m_maxLoopCount) {
                    m_currentFrame = m_endFrame;
                } else {
                    ++m_loopCount;
                }
            }
        }
        if(m_currentFrame >= m_maxFrames) {
            m_currentFrame = m_maxFrames - 1;
        }
    }
    m_frameDuration += deltaSeconds;
    m_elapsedDuration += deltaSeconds;
    if(m_elapsedDuration >= m_totalDuration) {
        m_elapsedDuration = TimeUtils::FPSeconds::zero();
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

void WebP::Render(const Matrix4& transform/* = Matrix4::I*/) const noexcept {
    PROFILE_BENCHMARK_FUNCTION();

    auto* r = ServiceLocator::get<IRendererService>();
    auto* mat = r->GetMaterial("__unlit2DSprite");
    if(const auto& cbs = mat->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        auto& cb = cbs[0].get();
        IntVector4 data{static_cast<int>(m_currentFrame), 0, 0, 0};
        cb.Update(*(r->GetDeviceContext()), &data);
    }
    r->SetMaterial(mat);
    r->SetTexture(m_frames.get());
    r->DrawQuad2D(transform);
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

Vector2 WebP::GetDimensions() const noexcept {
    return Vector2{static_cast<float>(m_width), static_cast<float>(m_height)};
}

void WebP::LoadFromXml(const XMLElement& elem) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    DataUtils::ValidateXmlElement(elem, "animation", "animationset", "", "", "name");

    const auto* xml_animset = elem.FirstChildElement("animationset");
    DataUtils::ValidateXmlElement(*xml_animset, "animationset", "", "src", "", "beginframe,endframe,loop,reverse,pingpong");

    if(const auto src = DataUtils::ParseXmlAttribute(*xml_animset, "src", std::string{}); !src.empty()) {
        LoadWebPData(src);
    }

    m_beginFrame = DataUtils::ParseXmlAttribute(*xml_animset, "beginframe", 0);
    m_endFrame = DataUtils::ParseXmlAttribute(*xml_animset, "endframe", m_frameCount);
    m_currentFrame = m_beginFrame;
    const auto is_looping = DataUtils::ParseXmlAttribute(*xml_animset, "loop", false);
    const auto is_reverse = DataUtils::ParseXmlAttribute(*xml_animset, "reverse", false);
    const auto is_pingpong = DataUtils::ParseXmlAttribute(*xml_animset, "pingpong", false);
    m_playback_mode = GetAnimModeFromOptions(is_looping, is_reverse, is_pingpong);

}

//TODO: Async?
void WebP::LoadWebPData(const std::filesystem::path& src) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(auto buffer = FileUtils::ReadBinaryBufferFromFile(src); buffer.has_value()) {

        WebPData webp_data{};
        webp_data.bytes = buffer->data();
        webp_data.size = buffer->size();

        WebPAnimDecoderOptions dec_options{};
        if(!WebPAnimDecoderOptionsInit(&dec_options)) {
            return;
        }
        dec_options.color_mode = MODE_RGBA;
        dec_options.use_threads = true;
        if(WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_options); dec != nullptr) {
            WebPAnimInfo anim_info{};
            if(!WebPAnimDecoderGetInfo(dec, &anim_info)) {
                return;
            }
            InitializeFromAnimInfo(anim_info);
            auto anim_frame_data = DecodeFrames(dec, anim_info);
            WebPAnimDecoderDelete(dec);
            dec = nullptr;
            buffer->clear();
            buffer->shrink_to_fit();
            auto* r = ServiceLocator::get<IRendererService>();
            m_frames = r->Create2DTextureArrayFromMemory(anim_frame_data.data(), anim_info.canvas_width, anim_info.canvas_height, anim_info.frame_count);
            anim_frame_data.clear();
            anim_frame_data.shrink_to_fit();
            m_frames->SetDebugName(src.string());
            m_totalDuration = std::reduce(std::execution::par_unseq, std::cbegin(m_frameDurations), std::cend(m_frameDurations), TimeUtils::FPSeconds::zero());
        }
    }
}

void WebP::InitializeFromAnimInfo(const WebPAnimInfo& anim_info) noexcept {
    m_frameDurations.resize(anim_info.frame_count);
    m_frameCount = anim_info.frame_count;
    m_maxFrames = m_frameCount;
    m_beginFrame = m_maxFrames - m_maxFrames;
    m_endFrame = m_maxFrames;
    m_currentFrame = m_beginFrame;
    m_width = anim_info.canvas_width;
    m_height = anim_info.canvas_height;
    m_maxLoopCount = anim_info.loop_count;
}

std::vector<uint8_t> WebP::DecodeFrames(WebPAnimDecoder* dec, const WebPAnimInfo& anim_info) {
    const auto row_offset = std::size_t{4u} * anim_info.canvas_width;
    const auto slice_offset = static_cast<std::size_t>(row_offset * anim_info.canvas_height);
    auto anim_frame_data = std::vector<uint8_t>(anim_info.frame_count * slice_offset);
    auto prev_timestamp = 0;
    for(int i = 0; WebPAnimDecoderHasMoreFrames(dec); ++i) {
        uint8_t* buf{};
        auto cur_timestamp = 0;
        if(WebPAnimDecoderGetNext(dec, &buf, &cur_timestamp)) {
            std::copy(buf, buf + slice_offset, anim_frame_data.begin() + (slice_offset * i));
            m_frameDurations[i] = std::chrono::milliseconds{cur_timestamp - prev_timestamp};
            prev_timestamp = cur_timestamp;
        }
    }
    return anim_frame_data;
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
