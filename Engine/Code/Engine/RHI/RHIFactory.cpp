#include "Engine/RHI/RHIFactory.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Rhi/RHIDevice.hpp"

RHIFactory::RHIFactory() noexcept {
#ifdef RENDER_DEBUG
    const auto hr_factory = ::CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_factory));
#else
    const auto hr_factory = ::CreateDXGIFactory2(0, IID_PPV_ARGS(&m_factory));
#endif
    GUARANTEE_OR_DIE(SUCCEEDED(hr_factory), "Failed to create factory for adapters.");
}

bool RHIFactory::QueryForAllowTearingSupport() const noexcept {
    namespace MWRL = Microsoft::WRL;
    BOOL allow_tearing{};
    if(auto hr_cfs = m_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing)); FAILED(hr_cfs)) {
        DebuggerPrintf(StringUtils::FormatWindowsMessage(hr_cfs));
        return false;
    }
    return allow_tearing == TRUE;
}

std::vector<AdapterInfo> RHIFactory::GetAdaptersByPreference(const AdapterPreference& preference) const noexcept {
    namespace MWRL = Microsoft::WRL;
    const auto dx_preference = AdapterPreferenceToDxgiGpuPreference(preference);
    std::vector<AdapterInfo> adapters{};
    Microsoft::WRL::ComPtr<IDXGIAdapter4> cur_adapter{};
    for(unsigned int i = 0u;
        SUCCEEDED(m_factory->EnumAdapterByGpuPreference(
        i,
        dx_preference,
        __uuidof(IDXGIAdapter4),
        &cur_adapter));
        ++i) {
        AdapterInfo cur_info{};
        cur_info.adapter = cur_adapter;
        cur_adapter->GetDesc3(&cur_info.desc);
        adapters.push_back(cur_info);
    }
    return adapters;
}

std::vector<AdapterInfo> RHIFactory::GetAdaptersByHighPerformancePreference() const noexcept {
    return GetAdaptersByPreference(AdapterPreference::HighPerformance);
}

std::vector<AdapterInfo> RHIFactory::GetAdaptersByMinimumPowerPreference() const noexcept {
    return GetAdaptersByPreference(AdapterPreference::MinimumPower);
}

std::vector<AdapterInfo> RHIFactory::GetAdaptersByUnspecifiedPreference() const noexcept {
    return GetAdaptersByPreference(AdapterPreference::Unspecified);
}

const IDXGIFactory7 * const RHIFactory::GetDxFactory() const noexcept {
    return m_factory.Get();
}

IDXGIFactory7* RHIFactory::GetDxFactory() noexcept {
    return m_factory.Get();
}
