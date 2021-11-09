#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/RHI/RHITypes.hpp"

class RHIDevice;
struct ID3D11RasterizerState;

struct RasterDesc {
    FillMode fillmode = FillMode::Solid;
    CullMode cullmode = CullMode::Back;
    float depthBiasClamp = 0.0f;
    float slopeScaledDepthBias = 0.0f;
    int depthBias = 0;
    bool depthClipEnable = true;
    bool scissorEnable = true;
    bool multisampleEnable = false;
    bool antialiasedLineEnable = false;
    bool frontCounterClockwise = false;
    RasterDesc() = default;
    explicit RasterDesc(const XMLElement& element) noexcept;
};

class RasterState {
public:
    RasterState(const RHIDevice* device, const RasterDesc& desc) noexcept;
    RasterState(const RHIDevice* device, const XMLElement& element) noexcept;
    ~RasterState() noexcept;

    [[nodiscard]] const RasterDesc& GetDesc() const noexcept;
    [[nodiscard]] ID3D11RasterizerState* GetDxRasterState() noexcept;

    void SetDebugName([[maybe_unused]] const std::string& name) const noexcept;

protected:
    [[nodiscard]] bool CreateRasterState(const RHIDevice* device, const RasterDesc& raster_desc = RasterDesc{}) noexcept;

private:
    RasterDesc _desc{};
    ID3D11RasterizerState* _dx_state{};
};