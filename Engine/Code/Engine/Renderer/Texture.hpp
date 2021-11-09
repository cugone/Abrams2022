#pragma once

#include "Engine/Math/IntVector3.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

#include <string>

class RHIDevice;

class Texture {
public:
    Texture() = delete;
    Texture(const RHIDevice& device) noexcept;
    Texture(Texture&& r_other) noexcept = default;
    Texture(const Texture& other) noexcept = delete;
    Texture& operator=(const Texture& rhs) noexcept = delete;
    Texture& operator=(Texture&& rhs) noexcept = default;
    virtual ~Texture() noexcept = 0;

    [[nodiscard]] const IntVector3& GetDimensions() const noexcept;

    void IsLoaded(bool is_loaded) noexcept;
    [[nodiscard]] bool IsLoaded() const noexcept;

    [[nodiscard]] bool IsArray() const noexcept;

    [[nodiscard]] ID3D11DepthStencilView* GetDepthStencilView() noexcept;
    [[nodiscard]] ID3D11RenderTargetView* GetRenderTargetView() noexcept;
    [[nodiscard]] ID3D11ShaderResourceView* GetShaderResourceView() noexcept;
    [[nodiscard]] ID3D11UnorderedAccessView* GetUnorderedAccessView() noexcept;
    [[nodiscard]] ID3D11DepthStencilView* GetDepthStencilView() const noexcept;
    [[nodiscard]] ID3D11RenderTargetView* GetRenderTargetView() const noexcept;
    [[nodiscard]] ID3D11ShaderResourceView* GetShaderResourceView() const noexcept;
    [[nodiscard]] ID3D11UnorderedAccessView* GetUnorderedAccessView() const noexcept;

    virtual void SetDebugName([[maybe_unused]] const std::string& name) const noexcept = 0;
    [[nodiscard]] virtual ID3D11Resource* GetDxResource() const noexcept = 0;

    template<typename T>
    [[nodiscard]] T* GetDxResourceAs() const noexcept {
        return static_cast<T*>(GetDxResource());
    }

protected:
    const RHIDevice& _device;
    IntVector3 _dimensions = IntVector3::Zero;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> _dsv{};
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _rtv{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _srv{};
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> _uav{};
    bool _isLoaded = false;
    bool _isArray = false;

private:
};
