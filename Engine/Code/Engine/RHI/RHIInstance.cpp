#include "Engine/RHI/RHIInstance.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/Window.hpp"

void RHIInstance::CreateDebugInstance() noexcept {
#if defined(RENDER_DEBUG)
    if(m_debuggerInstance) {
        return;
    }
    m_debuggerInstance = nullptr;

    // Debug Setup
    if(HMODULE debug_module = ::LoadLibraryA("Dxgidebug.dll"); debug_module != nullptr) {
        using GetDebugModuleCB = HRESULT(WINAPI*)(REFIID, void**);
        GetDebugModuleCB cb = (GetDebugModuleCB)::GetProcAddress(debug_module, "DXGIGetDebugInterface");
        HRESULT hr = cb(__uuidof(IDXGIDebug), reinterpret_cast<void**>(m_debuggerInstance.GetAddressOf()));
        bool succeeded = SUCCEEDED(hr);
        ASSERT_OR_DIE(succeeded, "DXGIDugger failed to initialize.");
        ReportLiveObjects();
    }
#endif
}

std::unique_ptr<RHIDevice> RHIInstance::CreateDevice() noexcept {
    return std::make_unique<RHIDevice>();
}

void RHIInstance::ReportLiveObjects() noexcept {
#ifdef RENDER_DEBUG
    if(m_debuggerInstance) {
        m_debuggerInstance->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
    }
#endif
}

RHIInstance::RHIInstance() noexcept {
#ifdef RENDER_DEBUG
    CreateDebugInstance();
#endif
}

RHIInstance::~RHIInstance() noexcept {
#ifdef RENDER_DEBUG
    ReportLiveObjects();
    m_debuggerInstance = nullptr;
#endif
}
