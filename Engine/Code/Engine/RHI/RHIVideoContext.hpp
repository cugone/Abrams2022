#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"

#include "Engine/RHI/RHIVideoDecoder.hpp"

#include <memory>

class RHIVideoContext {
public:
    RHIVideoContext() noexcept;
    ~RHIVideoContext() noexcept;

    bool Initialize() noexcept;
    void BeginFrame() noexcept;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render() const noexcept;
    void EndFrame() noexcept;

    ID3D11VideoContext3* GetDXContext() noexcept;
    RHIVideoDecoder* GetVideoDecoder() noexcept;

    void CreateVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept;

    protected:
private:
    Microsoft::WRL::ComPtr<ID3D11VideoContext3> m_dx_videocontext{};
    std::unique_ptr<RHIVideoDecoder> m_videoDecoder{};
    Microsoft::WRL::ComPtr<ID3D11VideoDecoderOutputView> m_dx_videodecoderOutput{};
};
