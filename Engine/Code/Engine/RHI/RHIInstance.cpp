#include "Engine/RHI/RHIInstance.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIDevice.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/Window.hpp"

RHIInstance* const RHIInstance::CreateInstance() noexcept {
    if(m_instance) {
        return m_instance;
    }
    m_instance = new RHIInstance();

    m_instance->m_debuggerInstance = nullptr;

#if defined(RENDER_DEBUG)

    HMODULE debug_module = nullptr;

    // Debug Setup

    debug_module = ::LoadLibraryA("Dxgidebug.dll");
    if(debug_module) {
        using GetDebugModuleCB = HRESULT(WINAPI*)(REFIID, void**);
        GetDebugModuleCB cb = (GetDebugModuleCB)::GetProcAddress(debug_module, "DXGIGetDebugInterface");
        HRESULT hr = cb(__uuidof(IDXGIDebug), reinterpret_cast<void**>(m_instance->m_debuggerInstance.GetAddressOf()));
        bool succeeded = SUCCEEDED(hr);
        ASSERT_OR_DIE(succeeded, "DXGIDugger failed to initialize.");
        m_instance->m_debuggerInstance->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
    }
#endif
    return m_instance;
}

void RHIInstance::DestroyInstance() noexcept {
    if(m_instance) {
        delete m_instance;
        m_instance = nullptr;
    }
}

std::unique_ptr<RHIDevice> RHIInstance::CreateDevice() noexcept {
    return std::make_unique<RHIDevice>();
}

void RHIInstance::ReportLiveObjects() noexcept {
#ifdef RENDER_DEBUG
    if(m_instance && m_instance->m_debuggerInstance) {
        m_instance->m_debuggerInstance->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
    }
#endif
}

RHIInstance::~RHIInstance() noexcept {
#ifdef RENDER_DEBUG
    if(m_instance && m_instance->m_debuggerInstance) {
        m_instance->m_debuggerInstance->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_IGNORE_INTERNAL | DXGI_DEBUG_RLO_DETAIL));
        m_instance->m_debuggerInstance = nullptr;
    }
#endif
}
