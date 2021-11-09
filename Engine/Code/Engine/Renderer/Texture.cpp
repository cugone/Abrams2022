#include "Engine/Renderer/Texture.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"

Texture::Texture(const RHIDevice& device) noexcept
: _device(device) {
    /* DO NOTHING */
}

Texture::~Texture() = default;

const IntVector3& Texture::GetDimensions() const noexcept {
    return _dimensions;
}

ID3D11Resource* Texture::GetDxResource() const noexcept {
    return nullptr;
}

void Texture::IsLoaded(bool is_loaded) noexcept {
    _isLoaded = is_loaded;
}

bool Texture::IsLoaded() const noexcept {
    return _isLoaded;
}

bool Texture::IsArray() const noexcept {
    return _isArray;
}

ID3D11DepthStencilView* Texture::GetDepthStencilView() const noexcept {
    return _dsv.Get();
}

ID3D11RenderTargetView* Texture::GetRenderTargetView() const noexcept {
    return _rtv.Get();
}

ID3D11ShaderResourceView* Texture::GetShaderResourceView() const noexcept {
    return _srv.Get();
}

ID3D11UnorderedAccessView* Texture::GetUnorderedAccessView() const noexcept {
    return _uav.Get();
}

ID3D11DepthStencilView* Texture::GetDepthStencilView() noexcept {
    return _dsv.Get();
}

ID3D11RenderTargetView* Texture::GetRenderTargetView() noexcept {
    return _rtv.Get();
}

ID3D11ShaderResourceView* Texture::GetShaderResourceView() noexcept {
    return _srv.Get();
}

ID3D11UnorderedAccessView* Texture::GetUnorderedAccessView() noexcept {
    return _uav.Get();
}
