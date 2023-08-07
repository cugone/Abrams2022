#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

class RHIVideoDecoder {
public:

    RHIVideoDecoder() = delete;
    RHIVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept;
    ~RHIVideoDecoder() noexcept = default;

    ID3D11VideoDecoder* GetDXDecoder() noexcept;

    VideoCreationDescription GetCreationParameters() const noexcept;
    void* GetDriverHandle() const noexcept;

protected:
private:
    Microsoft::WRL::ComPtr<ID3D11VideoDecoder> m_dx_videodecoder{};
    VideoCreationDescription m_desc{};
};
