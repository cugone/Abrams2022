#include "Engine/RHI/RHIOutput.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIDeviceContext.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Texture2D.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <sstream>

RHIOutput::RHIOutput(const RHIDevice& parent, std::unique_ptr<Window> wnd) noexcept
: m_parent_device(parent)
, m_window(std::move(wnd)) {
    SetDisplayMode(m_window->GetDisplayMode());
    CreateBuffers();
}

const RHIDevice& RHIOutput::GetParentDevice() const noexcept {
    return m_parent_device;
}

const Window* RHIOutput::GetWindow() const noexcept {
    return m_window.get();
}

Window* RHIOutput::GetWindow() noexcept {
    return m_window.get();
}

Texture* RHIOutput::GetBackBuffer() const noexcept {
    return m_backbuffer->GetTexture();
}

Texture* RHIOutput::GetDepthStencil() const noexcept {
    return m_backbuffer->GetDepthStencil();
}

IntVector2 RHIOutput::GetDimensions() const noexcept {
    if(m_window) {
        return m_window->GetClientDimensions();
    } else {
        return IntVector2::Zero;
    }
}

IntVector2 RHIOutput::GetCenter() const noexcept {
    const auto dims = GetDimensions();
    return dims / 2.0f;
}

float RHIOutput::GetAspectRatio() const noexcept {
    if(m_window) {
        const auto& dims = GetDimensions();
        if(dims.y < dims.x) {
            return dims.x / static_cast<float>(dims.y);
        } else {
            return dims.y / static_cast<float>(dims.x);
        }
    }
    return 0.0f;
}

void RHIOutput::SetDisplayMode(const RHIOutputMode& newMode) noexcept {
    m_window->SetDisplayMode(newMode);
}

void RHIOutput::SetDimensions(const IntVector2& clientSize) noexcept {
    m_window->SetDimensions(clientSize);
    m_backbuffer->Resize(clientSize.x, clientSize.y);
}

void RHIOutput::Present(bool vsync) noexcept {
    DXGI_PRESENT_PARAMETERS present_params{};
    present_params.DirtyRectsCount = 0;
    present_params.pDirtyRects = nullptr;
    present_params.pScrollOffset = nullptr;
    present_params.pScrollRect = nullptr;
    const auto allowed_to_tear = m_parent_device.IsAllowTearingSupported();
    const auto is_vsync_off = !vsync;
    const auto is_fullscreen = m_window->IsFullscreen();
    const auto is_not_fullscreen = !is_fullscreen;
    const auto use_no_sync_interval = allowed_to_tear && is_vsync_off;
    const auto sync_interval = use_no_sync_interval ? 0u : 1u;
    const auto can_tear = use_no_sync_interval && is_not_fullscreen;
    const auto present_flags = can_tear ? DXGI_PRESENT_ALLOW_TEARING : 0ul;
    if(const auto hr_present = m_parent_device.GetDxSwapChain()->Present1(sync_interval, present_flags, &present_params); FAILED(hr_present)) {
        switch(hr_present) {
        case DXGI_ERROR_DEVICE_REMOVED:
            [[fallthrough]];
        case DXGI_ERROR_DEVICE_RESET: {
            m_parent_device.HandleDeviceLost();
            const auto hr_removed_reset = m_parent_device.GetDxDevice()->GetDeviceRemovedReason();
            const auto err_str = std::format("Your GPU device has been lost. Please restart the application. The returned error message follows:\n{}", StringUtils::FormatWindowsMessage(hr_removed_reset));
            ERROR_AND_DIE(err_str.c_str());
            break;
        }
        default:
#ifdef RENDER_DEBUG
            const auto err_str = std::string{"Present call failed: "} + StringUtils::FormatWindowsMessage(hr_present);
            this->GetWindow()->Hide();
            GUARANTEE_OR_DIE(SUCCEEDED(hr_present), err_str.c_str());
#else
            this->GetWindow()->Hide();
            GUARANTEE_OR_DIE(SUCCEEDED(hr_present), "Present call failed.");
#endif
            break;
        }
    }
}

void RHIOutput::CreateBuffers() noexcept {
    DXGI_SWAP_CHAIN_DESC1 dxdesc{};
    if(auto result = m_parent_device.GetDxSwapChain()->GetDesc1(&dxdesc); FAILED(result)) {
        return;
    }
    FrameBufferDesc fbdesc{};
    fbdesc.SwapChainTarget = true;
    fbdesc.width = dxdesc.Width;
    fbdesc.height = dxdesc.Height;
    fbdesc.format = DxgiFormatToImageFormat(dxdesc.Format);
    fbdesc.samples = dxdesc.SampleDesc.Count;
    m_backbuffer = FrameBuffer::Create(fbdesc);
    m_backbuffer->GetTexture()->SetDebugName("__back_buffer");
    m_backbuffer->GetDepthStencil()->SetDebugName("__default_depthstencil");
}

void RHIOutput::SetTitle(const std::string& newTitle) const noexcept {
    m_window->SetTitle(newTitle);
}

void RHIOutput::ResetBackbuffer() noexcept {
    if(auto* renderer = ServiceLocator::get<IRendererService>(); renderer) {
        renderer->GetDeviceContext()->ClearState();
    }
    m_backbuffer->Resize(0u, 0u);
    m_parent_device.ResetSwapChainForHWnd();
    const auto [w, h] = m_window->GetClientDimensions();
    m_backbuffer->Resize(w, h);
}
