#pragma once

#include "Engine/RHI/RHITypes.hpp"
#include "Engine/Renderer/DirectX/DX11.hpp"

#include <memory>

class RHIOutput;
class IntVector2;
class RHIDevice;
class Renderer;

class RHIInstance {
public:
    RHIInstance() noexcept;
    ~RHIInstance() noexcept;

    [[nodiscard]] std::unique_ptr<RHIDevice> CreateDevice() noexcept;
    static void ReportLiveObjects() noexcept;

protected:
    void CreateDebugInstance() noexcept;

private:
    static inline Microsoft::WRL::ComPtr<IDXGIDebug> m_debuggerInstance = nullptr;
};