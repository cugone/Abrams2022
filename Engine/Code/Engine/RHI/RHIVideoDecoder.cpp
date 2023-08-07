#include "Engine/RHI/RHIVideoDecoder.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IVideoService.hpp"

RHIVideoDecoder::RHIVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept {
    GUARANTEE_OR_DIE(ValidateImageFormatForVideo(outputFormat), "Video Decoder created with image format that is not supported for video.");
    auto* v = ServiceLocator::get<IVideoService>();
    auto* videodevice = v->GetVideoDevice();
    auto* dx_videodevice = videodevice->GetDXDevice();

    D3D11_VIDEO_DECODER_DESC desc{};
    desc.SampleWidth = outputWidth;
    desc.SampleHeight = outputHeight;
    desc.OutputFormat = ImageFormatToDxgiFormat(outputFormat);
    const auto profile_count = dx_videodevice->GetVideoDecoderProfileCount();
    {
        bool profile_succeeded = false;
        for(unsigned int i = 0u; i < profile_count; ++i) {
            const auto hr_profile = dx_videodevice->GetVideoDecoderProfile(i, &desc.Guid);
            if(const auto success = !!SUCCEEDED(hr_profile); success) {
                profile_succeeded = success;
            }
        }
        GUARANTEE_OR_DIE(profile_succeeded, "Could not initialize video decoder profile.");
    }
    unsigned int config_count{0u};
    {
        const auto hr_config_count = dx_videodevice->GetVideoDecoderConfigCount(&desc, &config_count);
        const auto hr_error = StringUtils::FormatWindowsMessage(hr_config_count);
        GUARANTEE_OR_DIE(SUCCEEDED(hr_config_count), hr_error.c_str());
    }

    D3D11_VIDEO_DECODER_CONFIG config{};
    {
        bool config_succeeded = false;
        for(unsigned int i = 0u; i < config_count; ++i) {
            const auto hr_config = dx_videodevice->GetVideoDecoderConfig(&desc, i, &config);
            if(const auto success = !!SUCCEEDED(hr_config); success) {
                config_succeeded = success;
            }
        }
        GUARANTEE_OR_DIE(config_succeeded, "Could not initialize video config.");
    }
    {
        const auto hr_videodecoder = dx_videodevice->CreateVideoDecoder(&desc, &config, m_dx_videodecoder.GetAddressOf());
        const auto hr_error = StringUtils::FormatWindowsMessage(hr_videodecoder);
        GUARANTEE_OR_DIE(SUCCEEDED(hr_videodecoder), hr_error.c_str());
        m_desc.desc = desc;
        m_desc.config = config;
    }

}

ID3D11VideoDecoder* RHIVideoDecoder::GetDXDecoder() noexcept {
    return m_dx_videodecoder.Get();
}

VideoCreationDescription RHIVideoDecoder::GetCreationParameters() const noexcept {
    VideoCreationDescription creation_desc{};
    m_dx_videodecoder->GetCreationParameters(&creation_desc.desc, &creation_desc.config);
    return creation_desc;
}

void* RHIVideoDecoder::GetDriverHandle() const noexcept {
    HANDLE handle{};
    if(auto hr = m_dx_videodecoder->GetDriverHandle(&handle); SUCCEEDED(hr)) {
        return handle;
    }
    return nullptr;
}
