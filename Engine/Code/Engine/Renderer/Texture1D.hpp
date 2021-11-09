#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Texture.hpp"

class RHIDevice;

class Texture1D : public Texture {
public:
    Texture1D() = delete;
    Texture1D(const RHIDevice& device, Microsoft::WRL::ComPtr<ID3D11Texture1D> dxTexture) noexcept;
    Texture1D(Texture1D&& r_other) noexcept = default;
    Texture1D(const Texture1D& other) noexcept = delete;
    Texture1D& operator=(const Texture1D& rhs) noexcept = delete;
    Texture1D& operator=(Texture1D&& rhs) noexcept = default;

    virtual void SetDebugName([[maybe_unused]] const std::string& name) const noexcept override;

    virtual ~Texture1D() noexcept = default;

    [[nodiscard]] virtual ID3D11Resource* GetDxResource() const noexcept override;

protected:
private:
    void SetTexture() noexcept;

    Microsoft::WRL::ComPtr<ID3D11Texture1D> _dx_tex{};
};