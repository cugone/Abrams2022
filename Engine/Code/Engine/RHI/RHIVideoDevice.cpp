#include "Engine/RHI/RHIVideoDevice.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

RHIVideoDevice::RHIVideoDevice() noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    auto* device = r->GetDevice();
    Microsoft::WRL::ComPtr<ID3D11Device5> dx_device{device->GetDxDevice()};
    auto hr_dxvideodevice = dx_device.As(&m_dx_videodevice);
    const auto hr_error = StringUtils::FormatWindowsMessage(hr_dxvideodevice);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_dxvideodevice), hr_error.c_str());
}

ID3D11VideoDevice2* RHIVideoDevice::GetDXDevice() noexcept {
    return m_dx_videodevice.Get();
}

void RHIVideoDevice::CreateVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept {
    m_decoder = std::make_unique<RHIVideoDecoder>(outputWidth, outputHeight, outputFormat);
}

RHIVideoDecoder* RHIVideoDevice::GetDecoder() const noexcept {
    return m_decoder.get();
}
