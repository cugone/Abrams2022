#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

#include <vector>

class Window;
class RHIDevice;

class RHIFactory {
public:
    RHIFactory() noexcept;
    ~RHIFactory() = default;

    void RestrictAltEnterToggle(const RHIDevice& device) noexcept;
    [[nodiscard]] Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChainForHwnd(const RHIDevice& device, const Window& window, const DXGI_SWAP_CHAIN_DESC1& swapchain_desc) noexcept;

    [[nodiscard]] bool QueryForAllowTearingSupport(const RHIDevice& device) const noexcept;

    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByPreference(const AdapterPreference& preference) const noexcept;
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByHighPerformancePreference() const noexcept;
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByMinimumPowerPreference() const noexcept;
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByUnspecifiedPreference() const noexcept;

protected:
private:
    Microsoft::WRL::ComPtr<IDXGIFactory6> _dxgi_factory{};
};
