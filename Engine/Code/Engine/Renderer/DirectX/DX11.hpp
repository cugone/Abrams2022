#pragma once

#ifdef _DEBUG
    #define INITGUID
#endif

#include <d3d11_4.h>
#include <dxgi1_6.h>

// DEBUG STUFF
#include <D3Dcommon.h>
#include <d3d11sdklayers.h>
#include <dxgidebug.h>

// LIBS
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11shader.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include "Engine/RHI/RHITypes.hpp"

#include <cstdint>
#include <string>
#include <wrl/client.h>

struct AdapterInfo {
    Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter{};
    DXGI_ADAPTER_DESC3 desc{};
};

enum class AdapterPreference {
    None,
    Unspecified = None,
    HighPerformance,
    MinimumPower
};

[[nodiscard]] DXGI_GPU_PREFERENCE AdapterPreferenceToDxgiGpuPreference(const AdapterPreference& preference);

[[nodiscard]] GraphicsCardDesc AdapterInfoToGraphicsCardDesc(const AdapterInfo& adapterInfo) noexcept;

struct OutputInfo {
    Microsoft::WRL::ComPtr<IDXGIOutput6> output{};
    DXGI_OUTPUT_DESC1 desc{};
};

struct DeviceInfo {
    D3D_FEATURE_LEVEL highest_supported_feature_level{};
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> dx_context{};
    Microsoft::WRL::ComPtr<ID3D11Device5> dx_device{};
};

using bitfield8_t = std::uint8_t;
using bitfield16_t = std::uint16_t;
using bitfield32_t = std::uint32_t;
using bitfield64_t = std::uint64_t;
[[nodiscard]] bitfield8_t GetFilterMaskFromModes(const FilterMode& minFilterMode, const FilterMode& magFilterMode, const FilterMode& mipFilterMode, const FilterComparisonMode& minMaxComparison) noexcept;

[[nodiscard]] D3D11_FILTER FilterModeToD3DFilter(const FilterMode& minFilterMode, const FilterMode& magFilterMode, const FilterMode& mipFilterMode, const FilterComparisonMode& minMaxComparison) noexcept;
[[nodiscard]] FilterMode FilterModeFromString(const char* str) noexcept;
[[nodiscard]] FilterMode FilterModeFromString(std::string str) noexcept;

[[nodiscard]] FilterComparisonMode FilterComparisonModeFromString(const char* str) noexcept;
[[nodiscard]] FilterComparisonMode FilterComparisonModeFromString(std::string str) noexcept;

[[nodiscard]] D3D11_TEXTURE_ADDRESS_MODE AddressModeToD3DAddressMode(const TextureAddressMode& address_mode) noexcept;
[[nodiscard]] TextureAddressMode TextureAddressModeFromString(const char* str) noexcept;
[[nodiscard]] TextureAddressMode TextureAddressModeFromString(std::string str) noexcept;

[[nodiscard]] D3D11_COMPARISON_FUNC ComparisonFunctionToD3DComparisonFunction(const ComparisonFunction& compareFunc) noexcept;
[[nodiscard]] ComparisonFunction D3DComparisonFunctionToComparisonFunction(D3D11_COMPARISON_FUNC d3dcompareFunc) noexcept;
[[nodiscard]] ComparisonFunction ComparisonFunctionFromString(std::string str) noexcept;
[[nodiscard]] ComparisonFunction ComparisonFunctionFromString(const char* str) noexcept;

[[nodiscard]] D3D11_STENCIL_OP StencilOperationToD3DStencilOperation(const StencilOperation& stencil_operation) noexcept;
[[nodiscard]] StencilOperation StencilOperationFromString(const char* str) noexcept;
[[nodiscard]] StencilOperation StencilOperationFromString(std::string str) noexcept;

[[nodiscard]] D3D11_USAGE BufferUsageToD3DUsage(const BufferUsage& usage) noexcept;
[[nodiscard]] D3D11_BIND_FLAG BufferBindUsageToD3DBindFlags(const BufferBindUsage& bindFlags) noexcept;
[[nodiscard]] D3D11_CPU_ACCESS_FLAG CPUAccessFlagFromUsage(const BufferUsage& usage) noexcept;

[[nodiscard]] D3D11_PRIMITIVE_TOPOLOGY PrimitiveTypeToD3dTopology(const PrimitiveType& topology) noexcept;

[[nodiscard]] DXGI_FORMAT ImageFormatToDxgiFormat(const ImageFormat& format) noexcept;
[[nodiscard]] ImageFormat DxgiFormatToImageFormat(DXGI_FORMAT format) noexcept;

[[nodiscard]] D3D11_BLEND BlendFactorToD3DBlendFactor(const BlendFactor& factor) noexcept;
[[nodiscard]] D3D11_BLEND_OP BlendOpToD3DBlendOp(const BlendOperation& op) noexcept;
[[nodiscard]] UINT8 BlendColorWriteEnableToD3DBlendColorWriteEnable(const BlendColorWriteEnable& rt_mask) noexcept;

[[nodiscard]] BlendFactor BlendFactorFromString(std::string str) noexcept;
[[nodiscard]] BlendOperation BlendOperationFromString(std::string str) noexcept;
[[nodiscard]] BlendColorWriteEnable BlendColorWriteEnableFromString(std::string str) noexcept;

[[nodiscard]] D3D11_FILL_MODE FillModeToD3DFillMode(const FillMode& fillmode) noexcept;
[[nodiscard]] D3D11_CULL_MODE CullModeToD3DCullMode(const CullMode& fillmode) noexcept;
[[nodiscard]] FillMode FillModeFromString(std::string str) noexcept;
[[nodiscard]] CullMode CullModeFromString(std::string str) noexcept;
[[nodiscard]] WindingOrder WindingOrderFromString(std::string str) noexcept;

[[nodiscard]] D3D11_RESOURCE_MISC_FLAG ResourceMiscFlagToD3DMiscFlag(const ResourceMiscFlag& flags) noexcept;

[[nodiscard]] std::string PipelineStageToString(const PipelineStage& stage) noexcept;
[[nodiscard]] PipelineStage PipelineStageFromString(std::string stage) noexcept;