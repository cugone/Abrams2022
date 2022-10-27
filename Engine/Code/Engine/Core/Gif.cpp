#include "Engine/Core/Gif.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IFileLoggerService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Thirdparty/stb/stb_image.h"

#include <numeric>

namespace FileUtils {

Gif::Gif(const GifDesc& desc) noexcept
: m_startFrame{desc.startFrame}
, m_endFrame{desc.endFrame}
, m_playMode{desc.playMode}
{
    if(m_startFrame > m_endFrame) {
        std::swap(m_startFrame, m_endFrame);
    }
    const bool succeeded = Load(desc.filepath);
    GUARANTEE_OR_DIE(succeeded, "Gif: Failed to load");
}

void Gif::SetPlayMode(const Gif::PlayMode& newMode) noexcept {
    m_playMode = newMode;
}

void Gif::SetStartFrame(const std::size_t& newStartFrame) noexcept {
    m_startFrame = newStartFrame;
    if(m_currentFrame < m_startFrame) {
        m_currentFrame = m_startFrame;
    }
}

void Gif::SetEndFrame(const std::size_t& newEndFrame) noexcept {
    m_endFrame = newEndFrame;
    if(m_currentFrame > m_endFrame) {
        m_currentFrame = m_endFrame;
    }
}

void Gif::SetFrameRange(const std::size_t& newStartFrame, const std::size_t& newEndFrame) noexcept {
    SetStartFrame(newStartFrame);
    SetEndFrame(newEndFrame);
}

bool Gif::Load(const GifDesc& desc) noexcept {
    SetFrameRange(desc.startFrame, desc.endFrame);
    SetPlayMode(desc.playMode);
    return Load(desc.filepath);
}

bool Gif::Load(std::filesystem::path filepath) noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    auto* logger = ServiceLocator::get<IFileLoggerService>();
    if(!std::filesystem::exists(filepath)) {
        logger->LogLineAndFlush(std::format("File: {} does not exist.", filepath.string()));
        return false;
    }
    {
        std::error_code ec{};
        if(filepath = std::filesystem::canonical(filepath, ec); ec) {
            logger->LogErrorLine(std::format("File: {} is inaccessible.", filepath.string()));
            return {};
        }
    }
    filepath.make_preferred();
    {
        const auto magicBuffer = FileUtils::ReadSomeBinaryBufferFromFile(filepath, 0, 6);
        if(const bool isGif = magicBuffer.has_value() && (*magicBuffer == "GIF89a" || *magicBuffer == "GIF87a"); !isGif) {
            logger->LogLineAndFlush(std::format("File: {} is not a .gif", filepath.string()));
            return false;
        }
    }

    if(const auto buffer = FileUtils::ReadBinaryBufferFromFile(filepath); !buffer.has_value()) {
        logger->LogLineAndFlush("Gif: failed to read binary buffer.\n");
        return false;
    } else {
        int width{0};
        int height{0};
        int frame_count{0};
        int comp{4};
        int* delays{nullptr};
        if(uint8_t* data = stbi_load_gif_from_memory((*buffer).data(), static_cast<int>((*buffer).size()), &delays, &width, &height, &frame_count, &comp, 4); data == nullptr) {
            logger->LogLineAndFlush(std::format("stbi failed to load .gif from file: {}", filepath.string()));
            return false;
        } else {
            m_frameDelays = std::vector<TimeUtils::FPMilliseconds>(delays, delays + frame_count);
            m_totalFrames = frame_count;
            m_endFrame = (std::min)(m_endFrame, m_totalFrames - std::size_t{1u});
            m_currentFrame = m_startFrame;
            if(m_playMode == PlayMode::PlayToBeginning || m_playMode == PlayMode::Reverse) {
                m_currentFrame = m_endFrame;
            }
            m_duration = TimeUtils::FPSeconds{std::accumulate(std::cbegin(m_frameDelays), std::cend(m_frameDelays), TimeUtils::FPMilliseconds::zero(), [&](const auto& a, const auto& b) { return TimeUtils::FPMilliseconds(a.count() + b.count()); })};
            if(m_texture = r->GetTexture(filepath.string()); m_texture != nullptr) {
                stbi_image_free(data);
                data = nullptr;
                return true;
            }
            if(auto texture = r->Create2DTextureArrayFromMemory(data, width, height, frame_count); texture != nullptr) {
                m_texture = texture.get();
                if(!r->RegisterTexture(filepath.string(), std::move(texture))) {
                    stbi_image_free(data);
                    data = nullptr;
                    logger->LogLineAndFlush(std::format("Failed to register texture from .gif file: {}", filepath.string()));
                    return false;
                }
            }
            stbi_image_free(data);
            data = nullptr;
        }
        return true;
    }
}

void Gif::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_frameDuration += deltaSeconds;
    if(m_frameDuration >= m_frameDelays[m_currentFrame]) {
        if(m_playMode == PlayMode::PlayToBeginning || m_playMode == PlayMode::Reverse) {
            --m_currentFrame;
        } else {
            ++m_currentFrame;
        }
        m_frameDuration = TimeUtils::FPSeconds::zero();
    }
    switch(m_playMode) {
    case PlayMode::PlayToBeginning:
        if(m_currentFrame > m_endFrame) {
            m_currentFrame = m_startFrame;
        }
        break;
    case PlayMode::PlayToEnd:
        if(m_currentFrame > m_endFrame) {
            m_currentFrame = m_endFrame;
        }
        break;
    case PlayMode::Loop:
        if(m_currentFrame > m_endFrame) {
            m_currentFrame = m_startFrame;
        }
        break;
    case PlayMode::Reverse:
        if(m_currentFrame > m_endFrame) {
            m_currentFrame = m_endFrame;
        }
        break;
    default:
        /* DO NOTHING */
        break;
    }
}

void Gif::Render(const Matrix4& transform /*= Matrix4::I*/) const noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    auto* mat = r->GetMaterial("__unlit2DSprite");
    if(const auto& cbs = mat->GetShader()->GetConstantBuffers(); !cbs.empty()) {
        auto& cb = cbs[0].get();
        IntVector4 data{static_cast<int>(m_currentFrame), 0, 0, 0};
        cb.Update(*(r->GetDeviceContext()), &data);
    }
    r->SetMaterial(mat);
    r->SetTexture(m_texture);
    r->DrawQuad2D(transform);
}

IntVector2 Gif::GetDimensions() const noexcept {
    return IntVector2{m_texture->GetDimensions().x, m_texture->GetDimensions().y};
}

void Gif::SetImage(std::filesystem::path newFilepath) noexcept {
    SetFrameRange(0u, static_cast<std::size_t>(-1));
    Load(newFilepath);
}

} // namespace FileUtils