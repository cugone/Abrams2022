#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"
#include "Engine/Renderer/Texture.hpp"

class RHIDevice;

class Texture3D : public Texture {
public:
    Texture3D() = delete;
    Texture3D(const RHIDevice& device, Microsoft::WRL::ComPtr<ID3D11Texture3D> dxTexture) noexcept;
    Texture3D(Texture3D&& r_other) noexcept = default;
    Texture3D(const Texture3D& other) noexcept = delete;
    Texture3D& operator=(const Texture3D& rhs) noexcept = delete;
    Texture3D& operator=(Texture3D&& rhs) noexcept = default;

    virtual void SetDebugName([[maybe_unused]] const std::string& name) const noexcept override;

    virtual ~Texture3D() noexcept = default;

    [[nodiscard]] virtual ID3D11Resource* GetDxResource() const noexcept override;

protected:
private:
    void SetTexture();

    Microsoft::WRL::ComPtr<ID3D11Texture3D> _dx_tex{};
};