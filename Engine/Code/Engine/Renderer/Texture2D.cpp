#include "Engine/Renderer/Texture2D.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

#include <string>

Texture2D::Texture2D(const RHIDevice& device, Microsoft::WRL::ComPtr<ID3D11Texture2D> dxTexture) noexcept
: Texture(device)
, m_dx_tex(dxTexture) {
    SetTexture();
}

void Texture2D::SetDebugName([[maybe_unused]] const std::string& name) const noexcept {
#ifdef RENDER_DEBUG
    m_dx_tex->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned int>(name.size()), name.data());
#endif
}

IntVector2 Texture2D::GetDimensions() const noexcept {
    return IntVector2(m_dimensions);
}

ID3D11Resource* Texture2D::GetDxResource() const noexcept {
    return m_dx_tex.Get();
}

ID3D11Texture2D* Texture2D::GetDxTexture() noexcept {
    return static_cast<ID3D11Texture2D*>(GetDxResource());
}

void Texture2D::SetTexture() noexcept {
    D3D11_TEXTURE2D_DESC t_desc;
    m_dx_tex->GetDesc(&t_desc);
    auto depth = t_desc.ArraySize;
    m_dimensions = IntVector3(t_desc.Width, t_desc.Height, depth == 1 ? 0 : depth);

    bool success = true;
    std::string error_str{"Set device and texture failed. Reasons:\n"};
    if(t_desc.BindFlags & D3D11_BIND_RENDER_TARGET) {
        auto hr = m_device.GetDxDevice()->CreateRenderTargetView(m_dx_tex.Get(), nullptr, &m_rtv);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }

    if(bool is_depthstencil = (t_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)) {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc{};
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Flags = 0u;
        bool is_renderable_depthstencil = is_depthstencil && (t_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE);
        desc.Format = is_renderable_depthstencil ? ImageFormatToDxgiFormat(ImageFormat::D32_Float)
                                                 : ImageFormatToDxgiFormat(ImageFormat::D24_UNorm_S8_UInt);
        auto hr = m_device.GetDxDevice()->CreateDepthStencilView(m_dx_tex.Get(), &desc, &m_dsv);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }

    if(t_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        if(m_dsv) {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
            desc.Format = DXGI_FORMAT_R32_FLOAT;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            auto hr = m_device.GetDxDevice()->CreateShaderResourceView(m_dx_tex.Get(), &desc, &m_srv);
            if(FAILED(hr)) {
                success &= false;
                error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
            }
        } else {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
            desc.Format = t_desc.Format;
            desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            auto hr = m_device.GetDxDevice()->CreateShaderResourceView(m_dx_tex.Get(), &desc, &m_srv);
            if(FAILED(hr)) {
                success &= false;
                error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
            }
        }
    }

    if(t_desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
        desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        desc.Format = t_desc.Format;

        auto hr = m_device.GetDxDevice()->CreateUnorderedAccessView(m_dx_tex.Get(), &desc, &m_uav);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }

    if(!success) {
        if(m_dsv) {
            m_dsv = nullptr;
        }
        if(m_rtv) {
            m_rtv = nullptr;
        }
        if(m_srv) {
            m_srv = nullptr;
        }
        if(m_uav) {
            m_uav = nullptr;
        }
        ERROR_AND_DIE(error_str.c_str());
    }
}
