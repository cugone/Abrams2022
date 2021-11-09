#include "Engine/Renderer/Texture3D.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

Texture3D::Texture3D(const RHIDevice& device, Microsoft::WRL::ComPtr<ID3D11Texture3D> dxTexture) noexcept
: Texture(device)
, _dx_tex(dxTexture) {
    SetTexture();
}

void Texture3D::SetDebugName([[maybe_unused]] const std::string& name) const noexcept {
#ifdef RENDER_DEBUG
    _dx_tex->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<unsigned int>(name.size()), name.data());
#endif
}

ID3D11Resource* Texture3D::GetDxResource() const noexcept {
    return _dx_tex.Get();
}

void Texture3D::SetTexture() {
    D3D11_TEXTURE3D_DESC t_desc{};
    _dx_tex->GetDesc(&t_desc);
    _dimensions = IntVector3(t_desc.Width, t_desc.Height, t_desc.Depth);
    _isArray = false;

    bool success = true;
    std::string error_str{"SetTexture failed. Reasons:\n"};
    if(t_desc.BindFlags & D3D11_BIND_RENDER_TARGET) {
        D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
        rtv_desc.Format = t_desc.Format;
        rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        rtv_desc.Texture3D.MipSlice = 0;
        rtv_desc.Texture3D.FirstWSlice = 0;
        rtv_desc.Texture3D.WSize = static_cast<unsigned int>(-1);
        auto hr = _device.GetDxDevice()->CreateRenderTargetView(_dx_tex.Get(), &rtv_desc, &_rtv);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }

    if(t_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = t_desc.Format;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srv_desc.Texture3D.MipLevels = t_desc.MipLevels;
        srv_desc.Texture3D.MostDetailedMip = 0;
        auto hr = _device.GetDxDevice()->CreateShaderResourceView(_dx_tex.Get(), &srv_desc, &_srv);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }

    GUARANTEE_OR_DIE(!(t_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL), "Cannot bind Texture3D as Depth Stencil!");

    if(t_desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
        uav_desc.Format = t_desc.Format;
        uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
        uav_desc.Texture3D.MipSlice = 0;
        uav_desc.Texture3D.FirstWSlice = 0;
        uav_desc.Texture3D.WSize = static_cast<unsigned int>(-1);
        auto hr = _device.GetDxDevice()->CreateUnorderedAccessView(_dx_tex.Get(), &uav_desc, &_uav);
        if(FAILED(hr)) {
            success &= false;
            error_str += StringUtils::FormatWindowsMessage(hr) + '\n';
        }
    }
    if(!success) {
        if(_dsv) {
            _dsv = nullptr;
        }
        if(_rtv) {
            _rtv = nullptr;
        }
        if(_srv) {
            _srv = nullptr;
        }
        if(_uav) {
            _uav = nullptr;
        }
        ERROR_AND_DIE(error_str.c_str());
    }
}
