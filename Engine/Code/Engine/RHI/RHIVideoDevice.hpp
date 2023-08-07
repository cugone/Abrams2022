#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

class RHIVideoDecoder;

class RHIVideoDevice {
public:
    RHIVideoDevice() noexcept;
    ~RHIVideoDevice() noexcept = default;

    ID3D11VideoDevice2* GetDXDevice() noexcept;

    void CreateVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept;
    RHIVideoDecoder* GetDecoder() const noexcept;

protected:
private:
    Microsoft::WRL::ComPtr<ID3D11VideoDevice2> m_dx_videodevice{};
    std::unique_ptr<RHIVideoDecoder> m_decoder{};
};
