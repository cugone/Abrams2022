#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/RHI/RHITypes.hpp"

class RHIDevice;
struct ID3D11SamplerState;

struct SamplerDesc {
    FilterMode min_filter = FilterMode::Point;
    FilterMode mag_filter = FilterMode::Point;
    FilterMode mip_filter = FilterMode::Point;
    FilterComparisonMode compare_mode = FilterComparisonMode::None;
    TextureAddressMode UaddressMode = TextureAddressMode::Wrap;
    TextureAddressMode VaddressMode = TextureAddressMode::Wrap;
    TextureAddressMode WaddressMode = TextureAddressMode::Wrap;
    Rgba borderColor = Rgba::White;
    ComparisonFunction compareFunc = ComparisonFunction::Never;
    unsigned int maxAnisotropicLevel = 8;
    float mipmapLODBias = 0.0f;
    float minLOD = (std::numeric_limits<float>::lowest)();
    float maxLOD = (std::numeric_limits<float>::max)();
    SamplerDesc() = default;
    explicit SamplerDesc(const XMLElement& element) noexcept;
};

class Sampler {
public:
    explicit Sampler(const RHIDevice* device, const SamplerDesc& desc) noexcept;
    explicit Sampler(const RHIDevice* device, const XMLElement& element) noexcept;
    ~Sampler() noexcept;
    [[nodiscard]] ID3D11SamplerState* GetDxSampler() const noexcept;
    void SetDebugName([[maybe_unused]] const std::string& name) const noexcept;

protected:
private:
    [[nodiscard]] bool CreateSamplerState(const RHIDevice* device, const SamplerDesc& desc = SamplerDesc()) noexcept;

    SamplerDesc _desc{};
    ID3D11SamplerState* _dx_state = nullptr;
};