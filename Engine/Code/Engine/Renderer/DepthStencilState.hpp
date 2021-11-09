#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/RHI/RHITypes.hpp"

struct ID3D11DepthStencilState;
class RHIDevice;

struct DepthStencilDesc {
    bool depth_enabled = true;
    bool depth_write = true;
    ComparisonFunction depth_comparison = ComparisonFunction::Less;
    bool stencil_enabled = false;
    bool stencil_read = true;
    bool stencil_write = true;
    StencilOperation stencil_failFrontOp = StencilOperation::Keep;
    StencilOperation stencil_failBackOp = StencilOperation::Keep;
    StencilOperation stencil_failDepthFrontOp = StencilOperation::Keep;
    StencilOperation stencil_failDepthBackOp = StencilOperation::Keep;
    StencilOperation stencil_passFrontOp = StencilOperation::Keep;
    StencilOperation stencil_passBackOp = StencilOperation::Keep;
    ComparisonFunction stencil_testFront = ComparisonFunction::Always;
    ComparisonFunction stencil_testBack = ComparisonFunction::Always;
    DepthStencilDesc() = default;
    explicit DepthStencilDesc(const XMLElement& element) noexcept;
};

class DepthStencilState {
public:
    explicit DepthStencilState(const RHIDevice* device, const DepthStencilDesc& desc) noexcept;
    explicit DepthStencilState(const RHIDevice* device, const XMLElement& element) noexcept;
    ~DepthStencilState() noexcept;
    [[nodiscard]] ID3D11DepthStencilState* GetDxDepthStencilState() const noexcept;
    [[nodiscard]] DepthStencilDesc GetDesc() const noexcept;
    void SetDebugName([[maybe_unused]] const std::string& name) const noexcept;

protected:
private:
    [[nodiscard]] bool CreateDepthStencilState(const RHIDevice* device, const DepthStencilDesc& desc = DepthStencilDesc{}) noexcept;

    DepthStencilDesc _desc{};
    ID3D11DepthStencilState* _dx_state = nullptr;
};