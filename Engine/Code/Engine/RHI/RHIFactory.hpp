#pragma once

#include "Engine/Renderer/DirectX/DX11.hpp"

#include <vector>

class Window;
class RHIDevice;

class RHIFactory {
public:
    RHIFactory() noexcept;
    ~RHIFactory() noexcept = default;

    [[nodiscard]] bool QueryForAllowTearingSupport() const noexcept;

    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByHighPerformancePreference() const noexcept;
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByMinimumPowerPreference() const noexcept;
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByUnspecifiedPreference() const noexcept;

    const IDXGIFactory7 * const GetDxFactory() const noexcept;
    IDXGIFactory7* GetDxFactory() noexcept;

protected:
    [[nodiscard]] std::vector<AdapterInfo> GetAdaptersByPreference(const AdapterPreference& preference) const noexcept;

private:
    Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory{};
};
