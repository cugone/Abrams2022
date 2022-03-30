#include "Engine/Renderer/Texture.hpp"

#include "Engine/Renderer/DirectX/DX11.hpp"

Texture::Texture(const RHIDevice& device) noexcept
: m_device(device) {
    /* DO NOTHING */
}

Texture::~Texture() = default;

const IntVector3& Texture::GetDimensions() const noexcept {
    return m_dimensions;
}

ID3D11Resource* Texture::GetDxResource() const noexcept {
    return nullptr;
}

void Texture::IsLoaded(bool is_loaded) noexcept {
    m_isLoaded = is_loaded;
}

bool Texture::IsLoaded() const noexcept {
    return m_isLoaded;
}

bool Texture::IsArray() const noexcept {
    return m_isArray;
}

ID3D11DepthStencilView* Texture::GetDepthStencilView() const noexcept {
    return m_dsv.Get();
}

ID3D11RenderTargetView* Texture::GetRenderTargetView() const noexcept {
    return m_rtv.Get();
}

ID3D11ShaderResourceView* Texture::GetShaderResourceView() const noexcept {
    return m_srv.Get();
}

ID3D11UnorderedAccessView* Texture::GetUnorderedAccessView() const noexcept {
    return m_uav.Get();
}

ID3D11DepthStencilView* Texture::GetDepthStencilView() noexcept {
    return m_dsv.Get();
}

ID3D11RenderTargetView* Texture::GetRenderTargetView() noexcept {
    return m_rtv.Get();
}

ID3D11ShaderResourceView* Texture::GetShaderResourceView() noexcept {
    return m_srv.Get();
}

ID3D11UnorderedAccessView* Texture::GetUnorderedAccessView() noexcept {
    return m_uav.Get();
}
