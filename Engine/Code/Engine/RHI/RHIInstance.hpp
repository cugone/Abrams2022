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
    [[nodiscard]] static RHIInstance* const CreateInstance() noexcept;
    static void DestroyInstance() noexcept;

    [[nodiscard]] std::unique_ptr<RHIDevice> CreateDevice() noexcept;
    static void ReportLiveObjects() noexcept;

protected:
    RHIInstance() = default;
    ~RHIInstance() noexcept;

private:
    static inline RHIInstance* _instance = nullptr;
    static inline Microsoft::WRL::ComPtr<IDXGIDebug> _debuggerInstance = nullptr;
};