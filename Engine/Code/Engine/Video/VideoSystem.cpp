#include "Engine/Video/VideoSystem.hpp"

#include "Engine/RHI/RHIVideoDevice.hpp"
#include "Engine/RHI/RHIVideoContext.hpp"
#include "Engine/RHI/RHIVideoDecoder.hpp"
#include "Engine/RHI/RHIOutput.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <format>

void VideoSystem::Initialize() noexcept {
    m_videoDevice = CreateVideoDevice();
    m_videoContext = CreateVideoContext();
}

void VideoSystem::BeginFrame() noexcept {
    if(!m_playingVideo) {
        return;
    }
    m_videoContext->BeginFrame();
}

void VideoSystem::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!m_playingVideo) {
        return;
    }
}

void VideoSystem::Render() const noexcept {
    if(!m_playingVideo) {
        return;
    }
}

void VideoSystem::EndFrame() noexcept {
    if(!m_playingVideo) {
        return;
    }
}

void VideoSystem::Play([[maybe_unused]] const std::filesystem::path& filepath) {
    if(m_playingVideo) {
        return;
    }

    const auto* r = ServiceLocator::get<IRendererService>();
    const auto* o = r->GetOutput();
    const auto dimensions = o->GetDimensions();

    if(const bool is_webm_extension = filepath.has_extension() && filepath.extension() == std::filesystem::path{".webm"}; !is_webm_extension) {
        const auto err_msg = std::format("{} is not a supported video format.", filepath.string());
        auto* logger = ServiceLocator::get<IFileLoggerService>();
        logger->LogLineAndFlush(err_msg);
        return;
    }

    m_videoDevice->CreateVideoDecoder(dimensions.x, dimensions.y, ImageFormat::Nv11);

    m_playingVideo = true;
}

RHIVideoDevice* VideoSystem::GetVideoDevice() noexcept {
    return m_videoDevice.get();
}

RHIVideoContext* VideoSystem::GetVideoContext() noexcept {
    return m_videoContext.get();
}

RHIVideoDecoder* VideoSystem::GetVideoDecoder() noexcept {
    return m_videoDevice->GetDecoder();
}

std::unique_ptr<RHIVideoDevice> VideoSystem::CreateVideoDevice() const noexcept {
    return std::make_unique<RHIVideoDevice>();
}

std::unique_ptr<RHIVideoContext> VideoSystem::CreateVideoContext() const noexcept {
    return std::make_unique<RHIVideoContext>();
}
