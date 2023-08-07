#pragma once

#include "Engine/Core/EngineSubsystem.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"

#include "Engine/Services/IVideoService.hpp"

#include <atomic>
#include <filesystem>
#include <memory>

class RHIVideoDevice;
class RHIVideoContext;
class RHIVideoDecoder;

class VideoSystem : public EngineSubsystem, public IVideoService {
public:
    VideoSystem() noexcept = default;
    virtual ~VideoSystem() noexcept = default;

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    void Play(const std::filesystem::path& filepath);

    RHIVideoDevice* GetVideoDevice() noexcept override;
    RHIVideoContext* GetVideoContext() noexcept override;
    RHIVideoDecoder* GetVideoDecoder() noexcept override;

 protected:
 private:
    std::unique_ptr<RHIVideoDevice> CreateVideoDevice() const noexcept;
    std::unique_ptr<RHIVideoContext> CreateVideoContext() const noexcept;

    std::unique_ptr<RHIVideoDevice> m_videoDevice{};
    std::unique_ptr<RHIVideoContext> m_videoContext{};
    std::atomic_bool m_playingVideo{false};
};
