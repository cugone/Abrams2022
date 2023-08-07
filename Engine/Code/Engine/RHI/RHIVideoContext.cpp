#include "Engine/RHI/RHIVideoContext.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/RHI/RHIDeviceContext.hpp"
#include "Engine/RHI/RHIVideoDevice.hpp"
#include "Engine/RHI/RHIVideoDecoder.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IVideoService.hpp"

RHIVideoContext::RHIVideoContext() noexcept {
    auto* r = ServiceLocator::get<IRendererService>();
    auto* context = r->GetDeviceContext();
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> dx_context{context->GetDxContext()};
    auto hr_dxvideocontext = dx_context.As(&m_dx_videocontext);
    const auto hr_error = StringUtils::FormatWindowsMessage(hr_dxvideocontext);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_dxvideocontext), hr_error.c_str());
}

RHIVideoContext::~RHIVideoContext() noexcept {
    m_dx_videodecoderOutput.Reset();
    m_videoDecoder.reset();
    m_dx_videocontext.Reset();
}

bool RHIVideoContext::Initialize() noexcept {
    return true;
}

void RHIVideoContext::BeginFrame() noexcept {
    auto* decoder = m_videoDecoder->GetDXDecoder();
    const auto hr_beginframe1 = m_dx_videocontext->DecoderBeginFrame1(decoder, m_dx_videodecoderOutput.Get(), 0u, nullptr, 0u, nullptr, nullptr);
    const auto hr_error = StringUtils::FormatWindowsMessage(hr_beginframe1);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_beginframe1), hr_error.c_str());
}

void RHIVideoContext::Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    
}

void RHIVideoContext::Render() const noexcept {
}

void RHIVideoContext::EndFrame() noexcept {
    auto* decoder = m_videoDecoder->GetDXDecoder();
    const auto hr_endframe = m_dx_videocontext->DecoderEndFrame(decoder);
    const auto hr_error = StringUtils::FormatWindowsMessage(hr_endframe);
    GUARANTEE_OR_DIE(SUCCEEDED(hr_endframe), hr_error.c_str());
}

void RHIVideoContext::CreateVideoDecoder(unsigned int outputWidth, unsigned int outputHeight, const ImageFormat& outputFormat) noexcept {
    m_videoDecoder = std::make_unique<RHIVideoDecoder>(outputWidth, outputHeight, outputFormat);
}

ID3D11VideoContext3* RHIVideoContext::GetDXContext() noexcept {
    return m_dx_videocontext.Get();
}

RHIVideoDecoder* RHIVideoContext::GetVideoDecoder() noexcept {
    return m_videoDecoder.get();
}
