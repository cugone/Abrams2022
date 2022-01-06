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

#include "Engine/Core/StringUtils.hpp"
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

[[nodiscard]] constexpr DXGI_GPU_PREFERENCE AdapterPreferenceToDxgiGpuPreference(const AdapterPreference& preference) noexcept {
    switch(preference) {
    case AdapterPreference::None: /* Also AdapterPreference::Unspecified */
        return DXGI_GPU_PREFERENCE_UNSPECIFIED;
    case AdapterPreference::HighPerformance:
        return DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    case AdapterPreference::MinimumPower:
        return DXGI_GPU_PREFERENCE_MINIMUM_POWER;
    default:
        return DXGI_GPU_PREFERENCE_UNSPECIFIED;
    }
}

[[nodiscard]] constexpr GraphicsCardDesc AdapterInfoToGraphicsCardDesc(const AdapterInfo& adapterInfo) noexcept {
    GraphicsCardDesc desc{};
    desc.Description = StringUtils::ConvertUnicodeToMultiByte(std::wstring(adapterInfo.desc.Description));
    desc.DeviceId = adapterInfo.desc.DeviceId;
    desc.VendorId = adapterInfo.desc.VendorId;
    desc.SubSysId = adapterInfo.desc.SubSysId;
    desc.Revision = adapterInfo.desc.Revision;
    desc.DedicatedSystemMemory = adapterInfo.desc.DedicatedSystemMemory;
    desc.DedicatedVideoMemory = adapterInfo.desc.DedicatedVideoMemory;
    desc.SharedSystemMemory = adapterInfo.desc.SharedSystemMemory;
    desc.is_software = (adapterInfo.desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
    desc.is_unspecified = (adapterInfo.desc.Flags | DXGI_ADAPTER_FLAG3_NONE) == 0;
    return desc;
}

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


[[nodiscard]] constexpr FilterMode FilterModeFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "point"sv || sv == "pt"sv) {
        return FilterMode::Point;
    } else if(sv == "linear"sv || sv == "ln"sv) {
        return FilterMode::Linear;
    } else if(sv == "anisotropic"sv || sv == "af"sv) {
        return FilterMode::Anisotropic;
    } else {
        return FilterMode::Point;
    }
}

[[nodiscard]] FilterMode FilterModeFromString(std::string str) noexcept;

[[nodiscard]] constexpr FilterComparisonMode FilterComparisonModeFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "none"sv) {
        return FilterComparisonMode::None;
    } else if(sv == "min"sv || sv == "minimum"sv) {
        return FilterComparisonMode::Minimum;
    } else if(sv == "max"sv || sv == "maximum"sv) {
        return FilterComparisonMode::Maximum;
    } else if(sv == "comp"sv || sv == "comparison"sv) {
        return FilterComparisonMode::Comparison;
    } else {
        return FilterComparisonMode::None;
    }
}

[[nodiscard]] FilterComparisonMode FilterComparisonModeFromString(std::string str) noexcept;

[[nodiscard]] constexpr D3D11_TEXTURE_ADDRESS_MODE AddressModeToD3DAddressMode(const TextureAddressMode& address_mode) noexcept {
    switch(address_mode) {
    case TextureAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
    case TextureAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
    case TextureAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TextureAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
    case TextureAddressMode::Mirror_Once: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    default: return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}

[[nodiscard]] constexpr TextureAddressMode D3DAddressModeToAddressMode(D3D11_TEXTURE_ADDRESS_MODE d3daddress_mode) noexcept {
    switch(d3daddress_mode) {
    case D3D11_TEXTURE_ADDRESS_WRAP: return TextureAddressMode::Wrap;
    case D3D11_TEXTURE_ADDRESS_MIRROR: return TextureAddressMode::Mirror;
    case D3D11_TEXTURE_ADDRESS_CLAMP: return TextureAddressMode::Clamp;
    case D3D11_TEXTURE_ADDRESS_BORDER: return TextureAddressMode::Border;
    case D3D11_TEXTURE_ADDRESS_MIRROR_ONCE: return TextureAddressMode::Mirror_Once;
    default: return TextureAddressMode::Wrap;
    }
}

[[nodiscard]] constexpr TextureAddressMode TextureAddressModeFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "wrap"sv) {
        return TextureAddressMode::Wrap;
    } else if(sv == "mirror"sv) {
        return TextureAddressMode::Mirror;
    } else if(sv == "clamp"sv) {
        return TextureAddressMode::Clamp;
    } else if(sv == "border"sv) {
        return TextureAddressMode::Border;
    } else if(sv == "once"sv || sv == "mirror_once"sv) {
        return TextureAddressMode::Mirror_Once;
    } else {
        return TextureAddressMode::Wrap;
    }
}

[[nodiscard]] TextureAddressMode TextureAddressModeFromString(std::string str) noexcept;

[[nodiscard]] constexpr D3D11_COMPARISON_FUNC ComparisonFunctionToD3DComparisonFunction(const ComparisonFunction& compareFunc) noexcept {
    switch(compareFunc) {
    case ComparisonFunction::Never: return D3D11_COMPARISON_NEVER;
    case ComparisonFunction::Less: return D3D11_COMPARISON_LESS;
    case ComparisonFunction::Equal: return D3D11_COMPARISON_EQUAL;
    case ComparisonFunction::Less_Equal: return D3D11_COMPARISON_LESS_EQUAL;
    case ComparisonFunction::Greater: return D3D11_COMPARISON_GREATER;
    case ComparisonFunction::Not_Equal: return D3D11_COMPARISON_NOT_EQUAL;
    case ComparisonFunction::Greater_Equal: return D3D11_COMPARISON_GREATER_EQUAL;
    case ComparisonFunction::Always: return D3D11_COMPARISON_ALWAYS;
    default: return D3D11_COMPARISON_NEVER;
    }
}

[[nodiscard]] constexpr ComparisonFunction D3DComparisonFunctionToComparisonFunction(D3D11_COMPARISON_FUNC d3dcompareFunc) noexcept {
    switch(d3dcompareFunc) {
    case D3D11_COMPARISON_NEVER: return ComparisonFunction::Never;
    case D3D11_COMPARISON_LESS: return ComparisonFunction::Less;
    case D3D11_COMPARISON_EQUAL: return ComparisonFunction::Equal;
    case D3D11_COMPARISON_LESS_EQUAL: return ComparisonFunction::Less_Equal;
    case D3D11_COMPARISON_GREATER: return ComparisonFunction::Greater;
    case D3D11_COMPARISON_NOT_EQUAL: return ComparisonFunction::Not_Equal;
    case D3D11_COMPARISON_GREATER_EQUAL: return ComparisonFunction::Greater_Equal;
    case D3D11_COMPARISON_ALWAYS: return ComparisonFunction::Always;
    default: return ComparisonFunction::Never;
    }
}

[[nodiscard]] constexpr ComparisonFunction ComparisonFunctionFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "never"sv || sv == "0"sv) {
        return ComparisonFunction::Never;
    } else if(sv == "less"sv || sv == "<"sv) {
        return ComparisonFunction::Less;
    } else if(sv == "equal"sv || sv == "=="sv) {
        return ComparisonFunction::Equal;
    } else if(sv == "lessequal"sv || sv == "le"sv || sv == "leq"sv || sv == "<="sv) {
        return ComparisonFunction::Less_Equal;
    } else if(sv == "greater"sv || sv == ">"sv) {
        return ComparisonFunction::Greater;
    } else if(sv == "notequal"sv || sv == "ne"sv || sv == "neq"sv || sv == "!="sv) {
        return ComparisonFunction::Not_Equal;
    } else if(sv == "greaterequal"sv || sv == "ge"sv || sv == "geq"sv || sv == ">="sv) {
        return ComparisonFunction::Greater_Equal;
    } else if(sv == "always"sv || sv == "1"sv) {
        return ComparisonFunction::Always;
    } else {
        return ComparisonFunction::Never;
    }
}

[[nodiscard]] ComparisonFunction ComparisonFunctionFromString(std::string str) noexcept;

[[nodiscard]] constexpr D3D11_STENCIL_OP StencilOperationToD3DStencilOperation(const StencilOperation& stencil_operation) noexcept {
    switch(stencil_operation) {
    case StencilOperation::Keep: return D3D11_STENCIL_OP_KEEP;
    case StencilOperation::Zero: return D3D11_STENCIL_OP_ZERO;
    case StencilOperation::Replace: return D3D11_STENCIL_OP_REPLACE;
    case StencilOperation::Increment_Clamp: return D3D11_STENCIL_OP_INCR_SAT;
    case StencilOperation::Decrement_Clamp: return D3D11_STENCIL_OP_DECR_SAT;
    case StencilOperation::Invert: return D3D11_STENCIL_OP_INVERT;
    case StencilOperation::Increment_Wrap: return D3D11_STENCIL_OP_INCR;
    case StencilOperation::Decrement_Wrap: return D3D11_STENCIL_OP_DECR;
    default: return D3D11_STENCIL_OP_KEEP;
    }
}

[[nodiscard]] constexpr StencilOperation StencilOperationFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "keep"sv) {
        return StencilOperation::Keep;
    } else if(sv == "zero"sv) {
        return StencilOperation::Zero;
    } else if(sv == "replace"sv) {
        return StencilOperation::Replace;
    } else if(sv == "incrementandclamp"sv || sv == "ic"sv || sv == "incc"sv || sv == "inc"sv) {
        return StencilOperation::Increment_Clamp;
    } else if(sv == "decrementandclamp"sv || sv == "dc"sv || sv == "decc"sv || sv == "dec"sv) {
        return StencilOperation::Decrement_Clamp;
    } else if(sv == "invert"sv || sv == "inv"sv) {
        return StencilOperation::Invert;
    } else if(sv == "incrementandwrap"sv || sv == "iw"sv || sv == "incw"sv || sv == "inw"sv) {
        return StencilOperation::Increment_Wrap;
    } else if(sv == "decrementandwrap"sv || sv == "dw"sv || sv == "decw"sv || sv == "dew"sv) {
        return StencilOperation::Decrement_Wrap;
    } else {
        return StencilOperation::Keep;
    }
}

[[nodiscard]] StencilOperation StencilOperationFromString(std::string str) noexcept;

[[nodiscard]] constexpr D3D11_USAGE BufferUsageToD3DUsage(const BufferUsage& usage) noexcept {
    switch(usage) {
    case BufferUsage::Default: return D3D11_USAGE_DEFAULT;
    case BufferUsage::Gpu: return D3D11_USAGE_DEFAULT;
    case BufferUsage::Static: return D3D11_USAGE_IMMUTABLE;
    case BufferUsage::Dynamic: return D3D11_USAGE_DYNAMIC;
    case BufferUsage::Staging: return D3D11_USAGE_STAGING;
    default: return D3D11_USAGE_DEFAULT;
    }
}

[[nodiscard]] constexpr D3D11_BIND_FLAG BufferBindUsageToD3DBindFlags(const BufferBindUsage& bindFlags) noexcept {
    return static_cast<D3D11_BIND_FLAG>(bindFlags);
}

[[nodiscard]] constexpr D3D11_CPU_ACCESS_FLAG CPUAccessFlagFromUsage(const BufferUsage& usage) noexcept {
    switch(usage) {
    case BufferUsage::Gpu: return D3D11_CPU_ACCESS_READ;
    case BufferUsage::Dynamic: return D3D11_CPU_ACCESS_WRITE;
    case BufferUsage::Static: return static_cast<D3D11_CPU_ACCESS_FLAG>(0U);
    case BufferUsage::Staging: return static_cast<D3D11_CPU_ACCESS_FLAG>(D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
    default: return static_cast<D3D11_CPU_ACCESS_FLAG>(0U);
    }
}

[[nodiscard]] constexpr D3D11_PRIMITIVE_TOPOLOGY PrimitiveTypeToD3dTopology(const PrimitiveType& topology) noexcept {
    switch(topology) {
    case PrimitiveType::None: return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    case PrimitiveType::Points: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case PrimitiveType::Lines: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case PrimitiveType::Lines_Adj: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
    case PrimitiveType::LinesStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case PrimitiveType::LinesStrip_Adj: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    case PrimitiveType::Triangles: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveType::Triangles_Adj: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
    case PrimitiveType::TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case PrimitiveType::TriangleStrip_Adj: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
    case PrimitiveType::Control_Point_PatchList_1: return D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_2: return D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_3: return D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_4: return D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_5: return D3D11_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_6: return D3D11_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_7: return D3D11_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_8: return D3D11_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_9: return D3D11_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_10: return D3D11_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_11: return D3D11_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_12: return D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_13: return D3D11_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_14: return D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_15: return D3D11_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_16: return D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_17: return D3D11_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_18: return D3D11_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_19: return D3D11_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_20: return D3D11_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_21: return D3D11_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_22: return D3D11_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_23: return D3D11_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_24: return D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_25: return D3D11_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_26: return D3D11_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_27: return D3D11_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_28: return D3D11_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_29: return D3D11_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_30: return D3D11_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_31: return D3D11_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST;
    case PrimitiveType::Control_Point_PatchList_32: return D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST;
    default: return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

[[nodiscard]] constexpr DXGI_FORMAT ImageFormatToDxgiFormat(const ImageFormat& format) noexcept {
    switch(format) {
    case ImageFormat::Unknown: return DXGI_FORMAT_UNKNOWN;
    case ImageFormat::R32G32B32A32_Typeless: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case ImageFormat::R32G32B32A32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case ImageFormat::R32G32B32A32_UInt: return DXGI_FORMAT_R32G32B32A32_UINT;
    case ImageFormat::R32G32B32A32_SInt: return DXGI_FORMAT_R32G32B32A32_SINT;
    case ImageFormat::R32G32B32_Typeless: return DXGI_FORMAT_R32G32B32_TYPELESS;
    case ImageFormat::R32G32B32_Float: return DXGI_FORMAT_R32G32B32_FLOAT;
    case ImageFormat::R32G32B32_UInt: return DXGI_FORMAT_R32G32B32_UINT;
    case ImageFormat::R32G32B32_SInt: return DXGI_FORMAT_R32G32B32_SINT;
    case ImageFormat::R16G16B16A16_Typeless: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case ImageFormat::R16G16B16A16_Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case ImageFormat::R16G16B16A16_UNorm: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case ImageFormat::R16G16B16A16_UInt: return DXGI_FORMAT_R16G16B16A16_UINT;
    case ImageFormat::R16G16B16A16_SNorm: return DXGI_FORMAT_R16G16B16A16_SNORM;
    case ImageFormat::R16G16B16A16_SInt: return DXGI_FORMAT_R16G16B16A16_SINT;
    case ImageFormat::R32G32_Typeless: return DXGI_FORMAT_R32G32_TYPELESS;
    case ImageFormat::R32G32_Float: return DXGI_FORMAT_R32G32_FLOAT;
    case ImageFormat::R32G32_UInt: return DXGI_FORMAT_R32G32_UINT;
    case ImageFormat::R32G32_SInt: return DXGI_FORMAT_R32G32_SINT;
    case ImageFormat::R32G8X24_Typeless: return DXGI_FORMAT_R32G8X24_TYPELESS;
    case ImageFormat::D32_Float_S8X24_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case ImageFormat::R32_Float_X8X24_Typeless: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case ImageFormat::X32_Typeless_G8X24_UInt: return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case ImageFormat::R10G10B10A2_Typeless: return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case ImageFormat::R10G10B10A2_UNorm: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case ImageFormat::R10G10B10A2_UInt: return DXGI_FORMAT_R10G10B10A2_UINT;
    case ImageFormat::R11G11B10_Float: return DXGI_FORMAT_R11G11B10_FLOAT;
    case ImageFormat::R8G8B8A8_Typeless: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case ImageFormat::R8G8B8A8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::R8G8B8A8_UNorm_Srgb: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case ImageFormat::R8G8B8A8_UInt: return DXGI_FORMAT_R8G8B8A8_UINT;
    case ImageFormat::R8G8B8A8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;
    case ImageFormat::R8G8B8A8_SInt: return DXGI_FORMAT_R8G8B8A8_SINT;
    case ImageFormat::R16G16_Typeless: return DXGI_FORMAT_R16G16_TYPELESS;
    case ImageFormat::R16G16_Float: return DXGI_FORMAT_R16G16_FLOAT;
    case ImageFormat::R16G16_UNorm: return DXGI_FORMAT_R16G16_UNORM;
    case ImageFormat::R16G16_UInt: return DXGI_FORMAT_R16G16_UINT;
    case ImageFormat::R16G16_SNorm: return DXGI_FORMAT_R16G16_SNORM;
    case ImageFormat::R16G16_SInt: return DXGI_FORMAT_R16G16_SINT;
    case ImageFormat::R32_Typeless: return DXGI_FORMAT_R32_TYPELESS;
    case ImageFormat::D32_Float: return DXGI_FORMAT_D32_FLOAT;
    case ImageFormat::R32_Float: return DXGI_FORMAT_R32_FLOAT;
    case ImageFormat::R32_UInt: return DXGI_FORMAT_R32_UINT;
    case ImageFormat::R32_SInt: return DXGI_FORMAT_R32_SINT;
    case ImageFormat::R24G8_Typeless: return DXGI_FORMAT_R24G8_TYPELESS;
    case ImageFormat::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case ImageFormat::R24_UNorm_X8_Typeless: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case ImageFormat::X24_Typeless_G8_UInt: return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    case ImageFormat::R8G8_Typeless: return DXGI_FORMAT_R8G8_TYPELESS;
    case ImageFormat::R8G8_UNorm: return DXGI_FORMAT_R8G8_UNORM;
    case ImageFormat::R8G8_UInt: return DXGI_FORMAT_R8G8_UINT;
    case ImageFormat::R8G8_SNorm: return DXGI_FORMAT_R8G8_SNORM;
    case ImageFormat::R8G8_SInt: return DXGI_FORMAT_R8G8_SINT;
    case ImageFormat::R16_Typeless: return DXGI_FORMAT_R16_TYPELESS;
    case ImageFormat::R16_Float: return DXGI_FORMAT_R16_FLOAT;
    case ImageFormat::D16_UNorm: return DXGI_FORMAT_D16_UNORM;
    case ImageFormat::R16_UNorm: return DXGI_FORMAT_R16_UNORM;
    case ImageFormat::R16_UInt: return DXGI_FORMAT_R16_UINT;
    case ImageFormat::R16_SNorm: return DXGI_FORMAT_R16_SNORM;
    case ImageFormat::R16_SInt: return DXGI_FORMAT_R16_SINT;
    case ImageFormat::R8_Typeless: return DXGI_FORMAT_R8_TYPELESS;
    case ImageFormat::R8_UNorm: return DXGI_FORMAT_R8_UNORM;
    case ImageFormat::R8_UInt: return DXGI_FORMAT_R8_UINT;
    case ImageFormat::R8_SNorm: return DXGI_FORMAT_R8_SNORM;
    case ImageFormat::R8_SInt: return DXGI_FORMAT_R8_SINT;
    case ImageFormat::A8_UNorm: return DXGI_FORMAT_A8_UNORM;
    case ImageFormat::R1_UNorm: return DXGI_FORMAT_R1_UNORM;
    case ImageFormat::R9G9B9E5_SharedExp: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case ImageFormat::R8G8_B8G8_UNorm: return DXGI_FORMAT_R8G8_B8G8_UNORM;
    case ImageFormat::G8R8_G8B8_UNorm: return DXGI_FORMAT_G8R8_G8B8_UNORM;
    case ImageFormat::BC1_Typeless: return DXGI_FORMAT_BC1_TYPELESS;
    case ImageFormat::BC1_UNorm: return DXGI_FORMAT_BC1_UNORM;
    case ImageFormat::BC1_UNorm_Srgb: return DXGI_FORMAT_BC1_UNORM_SRGB;
    case ImageFormat::BC2_Typeless: return DXGI_FORMAT_BC2_TYPELESS;
    case ImageFormat::BC2_UNorm: return DXGI_FORMAT_BC2_UNORM;
    case ImageFormat::BC2_UNorm_Srgb: return DXGI_FORMAT_BC2_UNORM_SRGB;
    case ImageFormat::BC3_Typeless: return DXGI_FORMAT_BC3_TYPELESS;
    case ImageFormat::BC3_UNorm: return DXGI_FORMAT_BC3_UNORM;
    case ImageFormat::BC3_UNorm_Srgb: return DXGI_FORMAT_BC3_UNORM_SRGB;
    case ImageFormat::BC4_Typeless: return DXGI_FORMAT_BC4_TYPELESS;
    case ImageFormat::BC4_UNorm: return DXGI_FORMAT_BC4_UNORM;
    case ImageFormat::BC4_SNorm: return DXGI_FORMAT_BC4_SNORM;
    case ImageFormat::BC5_Typeless: return DXGI_FORMAT_BC5_TYPELESS;
    case ImageFormat::BC5_UNorm: return DXGI_FORMAT_BC5_UNORM;
    case ImageFormat::BC5_SNorm: return DXGI_FORMAT_BC5_SNORM;
    case ImageFormat::B5G6R5_UNorm: return DXGI_FORMAT_B5G6R5_UNORM;
    case ImageFormat::B5G5R5A1_UNorm: return DXGI_FORMAT_B5G5R5A1_UNORM;
    case ImageFormat::B8G8R8A8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat::B8G8R8X8_UNorm: return DXGI_FORMAT_B8G8R8X8_UNORM;
    case ImageFormat::R10G10B10_XR_Bias_A2_UNorm: return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    case ImageFormat::B8G8R8A8_Typeless: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case ImageFormat::B8G8R8A8_UNorm_Srgb: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case ImageFormat::B8G8R8X8_Typeless: return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case ImageFormat::B8G8R8X8_UNorm_Srgb: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case ImageFormat::BC6H_Typeless: return DXGI_FORMAT_BC6H_TYPELESS;
    case ImageFormat::BC6H_UF16: return DXGI_FORMAT_BC6H_UF16;
    case ImageFormat::BC6H_SF16: return DXGI_FORMAT_BC6H_SF16;
    case ImageFormat::BC7_Typeless: return DXGI_FORMAT_BC7_TYPELESS;
    case ImageFormat::BC7_UNorm: return DXGI_FORMAT_BC7_UNORM;
    case ImageFormat::BC7_UNorm_Srgb: return DXGI_FORMAT_BC7_UNORM_SRGB;
    case ImageFormat::Ayuv: return DXGI_FORMAT_AYUV;
    case ImageFormat::Y410: return DXGI_FORMAT_Y410;
    case ImageFormat::Y416: return DXGI_FORMAT_Y416;
    case ImageFormat::Nv12: return DXGI_FORMAT_NV12;
    case ImageFormat::P010: return DXGI_FORMAT_P010;
    case ImageFormat::P016: return DXGI_FORMAT_P016;
    case ImageFormat::Opaque_420: return DXGI_FORMAT_420_OPAQUE;
    case ImageFormat::Yuy2: return DXGI_FORMAT_YUY2;
    case ImageFormat::Y210: return DXGI_FORMAT_Y210;
    case ImageFormat::Y216: return DXGI_FORMAT_Y216;
    case ImageFormat::Nv11: return DXGI_FORMAT_NV11;
    case ImageFormat::Ai44: return DXGI_FORMAT_AI44;
    case ImageFormat::Ia44: return DXGI_FORMAT_IA44;
    case ImageFormat::P8: return DXGI_FORMAT_P8;
    case ImageFormat::A8P8: return DXGI_FORMAT_A8P8;
    case ImageFormat::B4G4R4A4_UNorm: return DXGI_FORMAT_B4G4R4A4_UNORM;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

[[nodiscard]] constexpr ImageFormat DxgiFormatToImageFormat(DXGI_FORMAT format) noexcept {
    switch(format) {
    case DXGI_FORMAT_UNKNOWN: return ImageFormat::Unknown;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: return ImageFormat::R32G32B32A32_Typeless;
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return ImageFormat::R32G32B32A32_Float;
    case DXGI_FORMAT_R32G32B32A32_UINT: return ImageFormat::R32G32B32A32_UInt;
    case DXGI_FORMAT_R32G32B32A32_SINT: return ImageFormat::R32G32B32A32_SInt;
    case DXGI_FORMAT_R32G32B32_TYPELESS: return ImageFormat::R32G32B32_Typeless;
    case DXGI_FORMAT_R32G32B32_FLOAT: return ImageFormat::R32G32B32_Float;
    case DXGI_FORMAT_R32G32B32_UINT: return ImageFormat::R32G32B32_UInt;
    case DXGI_FORMAT_R32G32B32_SINT: return ImageFormat::R32G32B32_SInt;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return ImageFormat::R16G16B16A16_Typeless;
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return ImageFormat::R16G16B16A16_Float;
    case DXGI_FORMAT_R16G16B16A16_UNORM: return ImageFormat::R16G16B16A16_UNorm;
    case DXGI_FORMAT_R16G16B16A16_UINT: return ImageFormat::R16G16B16A16_UInt;
    case DXGI_FORMAT_R16G16B16A16_SNORM: return ImageFormat::R16G16B16A16_SNorm;
    case DXGI_FORMAT_R16G16B16A16_SINT: return ImageFormat::R16G16B16A16_SInt;
    case DXGI_FORMAT_R32G32_TYPELESS: return ImageFormat::R32G32_Typeless;
    case DXGI_FORMAT_R32G32_FLOAT: return ImageFormat::R32G32_Float;
    case DXGI_FORMAT_R32G32_UINT: return ImageFormat::R32G32_UInt;
    case DXGI_FORMAT_R32G32_SINT: return ImageFormat::R32G32_SInt;
    case DXGI_FORMAT_R32G8X24_TYPELESS: return ImageFormat::R32G8X24_Typeless;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return ImageFormat::D32_Float_S8X24_UInt;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return ImageFormat::R32_Float_X8X24_Typeless;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: return ImageFormat::X32_Typeless_G8X24_UInt;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return ImageFormat::R10G10B10A2_Typeless;
    case DXGI_FORMAT_R10G10B10A2_UNORM: return ImageFormat::R10G10B10A2_UNorm;
    case DXGI_FORMAT_R10G10B10A2_UINT: return ImageFormat::R10G10B10A2_UInt;
    case DXGI_FORMAT_R11G11B10_FLOAT: return ImageFormat::R11G11B10_Float;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return ImageFormat::R8G8B8A8_Typeless;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return ImageFormat::R8G8B8A8_UNorm;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return ImageFormat::R8G8B8A8_UNorm_Srgb;
    case DXGI_FORMAT_R8G8B8A8_UINT: return ImageFormat::R8G8B8A8_UInt;
    case DXGI_FORMAT_R8G8B8A8_SNORM: return ImageFormat::R8G8B8A8_SNorm;
    case DXGI_FORMAT_R8G8B8A8_SINT: return ImageFormat::R8G8B8A8_SInt;
    case DXGI_FORMAT_R16G16_TYPELESS: return ImageFormat::R16G16_Typeless;
    case DXGI_FORMAT_R16G16_FLOAT: return ImageFormat::R16G16_Float;
    case DXGI_FORMAT_R16G16_UNORM: return ImageFormat::R16G16_UNorm;
    case DXGI_FORMAT_R16G16_UINT: return ImageFormat::R16G16_UInt;
    case DXGI_FORMAT_R16G16_SNORM: return ImageFormat::R16G16_SNorm;
    case DXGI_FORMAT_R16G16_SINT: return ImageFormat::R16G16_SInt;
    case DXGI_FORMAT_R32_TYPELESS: return ImageFormat::R32_Typeless;
    case DXGI_FORMAT_D32_FLOAT: return ImageFormat::D32_Float;
    case DXGI_FORMAT_R32_FLOAT: return ImageFormat::R32_Float;
    case DXGI_FORMAT_R32_UINT: return ImageFormat::R32_UInt;
    case DXGI_FORMAT_R32_SINT: return ImageFormat::R32_SInt;
    case DXGI_FORMAT_R24G8_TYPELESS: return ImageFormat::R24G8_Typeless;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return ImageFormat::D24_UNorm_S8_UInt;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: return ImageFormat::R24_UNorm_X8_Typeless;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return ImageFormat::X24_Typeless_G8_UInt;
    case DXGI_FORMAT_R8G8_TYPELESS: return ImageFormat::R8G8_Typeless;
    case DXGI_FORMAT_R8G8_UNORM: return ImageFormat::R8G8_UNorm;
    case DXGI_FORMAT_R8G8_UINT: return ImageFormat::R8G8_UInt;
    case DXGI_FORMAT_R8G8_SNORM: return ImageFormat::R8G8_SNorm;
    case DXGI_FORMAT_R8G8_SINT: return ImageFormat::R8G8_SInt;
    case DXGI_FORMAT_R16_TYPELESS: return ImageFormat::R16_Typeless;
    case DXGI_FORMAT_R16_FLOAT: return ImageFormat::R16_Float;
    case DXGI_FORMAT_D16_UNORM: return ImageFormat::D16_UNorm;
    case DXGI_FORMAT_R16_UNORM: return ImageFormat::R16_UNorm;
    case DXGI_FORMAT_R16_UINT: return ImageFormat::R16_UInt;
    case DXGI_FORMAT_R16_SNORM: return ImageFormat::R16_SNorm;
    case DXGI_FORMAT_R16_SINT: return ImageFormat::R16_SInt;
    case DXGI_FORMAT_R8_TYPELESS: return ImageFormat::R8_Typeless;
    case DXGI_FORMAT_R8_UNORM: return ImageFormat::R8_UNorm;
    case DXGI_FORMAT_R8_UINT: return ImageFormat::R8_UInt;
    case DXGI_FORMAT_R8_SNORM: return ImageFormat::R8_SNorm;
    case DXGI_FORMAT_R8_SINT: return ImageFormat::R8_SInt;
    case DXGI_FORMAT_A8_UNORM: return ImageFormat::A8_UNorm;
    case DXGI_FORMAT_R1_UNORM: return ImageFormat::R1_UNorm;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: return ImageFormat::R9G9B9E5_SharedExp;
    case DXGI_FORMAT_R8G8_B8G8_UNORM: return ImageFormat::R8G8_B8G8_UNorm;
    case DXGI_FORMAT_G8R8_G8B8_UNORM: return ImageFormat::G8R8_G8B8_UNorm;
    case DXGI_FORMAT_BC1_TYPELESS: return ImageFormat::BC1_Typeless;
    case DXGI_FORMAT_BC1_UNORM: return ImageFormat::BC1_UNorm;
    case DXGI_FORMAT_BC1_UNORM_SRGB: return ImageFormat::BC1_UNorm_Srgb;
    case DXGI_FORMAT_BC2_TYPELESS: return ImageFormat::BC2_Typeless;
    case DXGI_FORMAT_BC2_UNORM: return ImageFormat::BC2_UNorm;
    case DXGI_FORMAT_BC2_UNORM_SRGB: return ImageFormat::BC2_UNorm_Srgb;
    case DXGI_FORMAT_BC3_TYPELESS: return ImageFormat::BC3_Typeless;
    case DXGI_FORMAT_BC3_UNORM: return ImageFormat::BC3_UNorm;
    case DXGI_FORMAT_BC3_UNORM_SRGB: return ImageFormat::BC3_UNorm_Srgb;
    case DXGI_FORMAT_BC4_TYPELESS: return ImageFormat::BC4_Typeless;
    case DXGI_FORMAT_BC4_UNORM: return ImageFormat::BC4_UNorm;
    case DXGI_FORMAT_BC4_SNORM: return ImageFormat::BC4_SNorm;
    case DXGI_FORMAT_BC5_TYPELESS: return ImageFormat::BC5_Typeless;
    case DXGI_FORMAT_BC5_UNORM: return ImageFormat::BC5_UNorm;
    case DXGI_FORMAT_BC5_SNORM: return ImageFormat::BC5_SNorm;
    case DXGI_FORMAT_B5G6R5_UNORM: return ImageFormat::B5G6R5_UNorm;
    case DXGI_FORMAT_B5G5R5A1_UNORM: return ImageFormat::B5G5R5A1_UNorm;
    case DXGI_FORMAT_B8G8R8A8_UNORM: return ImageFormat::B8G8R8A8_UNorm;
    case DXGI_FORMAT_B8G8R8X8_UNORM: return ImageFormat::B8G8R8X8_UNorm;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return ImageFormat::R10G10B10_XR_Bias_A2_UNorm;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return ImageFormat::B8G8R8A8_Typeless;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return ImageFormat::B8G8R8A8_UNorm_Srgb;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: return ImageFormat::B8G8R8X8_Typeless;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return ImageFormat::B8G8R8X8_UNorm_Srgb;
    case DXGI_FORMAT_BC6H_TYPELESS: return ImageFormat::BC6H_Typeless;
    case DXGI_FORMAT_BC6H_UF16: return ImageFormat::BC6H_UF16;
    case DXGI_FORMAT_BC6H_SF16: return ImageFormat::BC6H_SF16;
    case DXGI_FORMAT_BC7_TYPELESS: return ImageFormat::BC7_Typeless;
    case DXGI_FORMAT_BC7_UNORM: return ImageFormat::BC7_UNorm;
    case DXGI_FORMAT_BC7_UNORM_SRGB: return ImageFormat::BC7_UNorm_Srgb;
    case DXGI_FORMAT_AYUV: return ImageFormat::Ayuv;
    case DXGI_FORMAT_Y410: return ImageFormat::Y410;
    case DXGI_FORMAT_Y416: return ImageFormat::Y416;
    case DXGI_FORMAT_NV12: return ImageFormat::Nv12;
    case DXGI_FORMAT_P010: return ImageFormat::P010;
    case DXGI_FORMAT_P016: return ImageFormat::P016;
    case DXGI_FORMAT_420_OPAQUE: return ImageFormat::Opaque_420;
    case DXGI_FORMAT_YUY2: return ImageFormat::Yuy2;
    case DXGI_FORMAT_Y210: return ImageFormat::Y210;
    case DXGI_FORMAT_Y216: return ImageFormat::Y216;
    case DXGI_FORMAT_NV11: return ImageFormat::Nv11;
    case DXGI_FORMAT_AI44: return ImageFormat::Ai44;
    case DXGI_FORMAT_IA44: return ImageFormat::Ia44;
    case DXGI_FORMAT_P8: return ImageFormat::P8;
    case DXGI_FORMAT_A8P8: return ImageFormat::A8P8;
    case DXGI_FORMAT_B4G4R4A4_UNORM: return ImageFormat::B4G4R4A4_UNorm;
    default: return ImageFormat::Unknown;
    }
}

[[nodiscard]] constexpr D3D11_BLEND BlendFactorToD3DBlendFactor(const BlendFactor& factor) noexcept {
    switch(factor) {
    case BlendFactor::Zero: return D3D11_BLEND_ZERO;
    case BlendFactor::One: return D3D11_BLEND_ONE;
    case BlendFactor::Src_Color: return D3D11_BLEND_SRC_COLOR;
    case BlendFactor::Inv_Src_Color: return D3D11_BLEND_INV_SRC_COLOR;
    case BlendFactor::Src_Alpha: return D3D11_BLEND_SRC_ALPHA;
    case BlendFactor::Inv_Src_Alpha: return D3D11_BLEND_INV_SRC_ALPHA;
    case BlendFactor::Dest_Alpha: return D3D11_BLEND_DEST_ALPHA;
    case BlendFactor::Inv_Dest_Alpha: return D3D11_BLEND_INV_DEST_ALPHA;
    case BlendFactor::Dest_Color: return D3D11_BLEND_DEST_COLOR;
    case BlendFactor::Inv_Dest_Color: return D3D11_BLEND_INV_DEST_COLOR;
    case BlendFactor::Src_Alpha_Sat: return D3D11_BLEND_SRC_ALPHA_SAT;
    case BlendFactor::Blend_Factor: return D3D11_BLEND_BLEND_FACTOR;
    case BlendFactor::Inv_Blend_Factor: return D3D11_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::Src1_Color: return D3D11_BLEND_SRC1_COLOR;
    case BlendFactor::Inv_Src1_Color: return D3D11_BLEND_INV_SRC1_COLOR;
    case BlendFactor::Src1_Alpha: return D3D11_BLEND_SRC1_ALPHA;
    case BlendFactor::Inv_Src1_Alpha: return D3D11_BLEND_INV_SRC1_ALPHA;
    default: return D3D11_BLEND_ONE;
    }
}

[[nodiscard]] constexpr D3D11_BLEND_OP BlendOpToD3DBlendOp(const BlendOperation& op) noexcept {
    switch(op) {
    case BlendOperation::Add: return D3D11_BLEND_OP_ADD;
    case BlendOperation::Subtract: return D3D11_BLEND_OP_SUBTRACT;
    case BlendOperation::Reverse_Subtract: return D3D11_BLEND_OP_REV_SUBTRACT;
    case BlendOperation::Min: return D3D11_BLEND_OP_MIN;
    case BlendOperation::Max: return D3D11_BLEND_OP_MAX;
    default: return D3D11_BLEND_OP_ADD;
    }
}

[[nodiscard]] constexpr UINT8 BlendColorWriteEnableToD3DBlendColorWriteEnable(const BlendColorWriteEnable& rt_mask) noexcept {
    return static_cast<UINT8>(rt_mask);
}

[[nodiscard]] constexpr BlendFactor BlendFactorFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "zero"sv) {
        return BlendFactor::Zero;
    } else if(sv == "one"sv) {
        return BlendFactor::One;
    } else if(sv == "src_color"sv) {
        return BlendFactor::Src_Color;
    } else if(sv == "inv_src_color"sv) {
        return BlendFactor::Inv_Src_Color;
    } else if(sv == "src_alpha"sv) {
        return BlendFactor::Src_Alpha;
    } else if(sv == "inv_src_alpha"sv) {
        return BlendFactor::Inv_Src_Alpha;
    } else if(sv == "dest_alpha"sv) {
        return BlendFactor::Dest_Alpha;
    } else if(sv == "inv_dest_alpha"sv) {
        return BlendFactor::Inv_Dest_Alpha;
    } else if(sv == "dest_color"sv) {
        return BlendFactor::Dest_Color;
    } else if(sv == "inv_dest_color"sv) {
        return BlendFactor::Inv_Dest_Color;
    } else if(sv == "src_alpha_sat"sv) {
        return BlendFactor::Src_Alpha_Sat;
    } else if(sv == "blend_factor"sv) {
        return BlendFactor::Blend_Factor;
    } else if(sv == "inv_blend_factor"sv) {
        return BlendFactor::Inv_Blend_Factor;
    } else if(sv == "src1_color"sv) {
        return BlendFactor::Src1_Color;
    } else if(sv == "inv_src1_color"sv) {
        return BlendFactor::Inv_Src1_Color;
    } else if(sv == "src1_alpha"sv) {
        return BlendFactor::Src1_Alpha;
    } else if(sv == "inv_src1_alpha"sv) {
        return BlendFactor::Inv_Src1_Alpha;
    } else {
        return BlendFactor::Zero;
    }
}

[[nodiscard]] BlendFactor BlendFactorFromString(std::string str) noexcept;

[[nodiscard]] constexpr BlendOperation BlendOperationFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "add"sv) {
        return BlendOperation::Add;
    } else if(sv == "subtract"sv || sv == "sub"sv) {
        return BlendOperation::Subtract;
    } else if(sv == "rev_sub"sv || sv == "rev_subtract"sv || sv == "reverse_sub"sv || sv == "reverse_subtract"sv) {
        return BlendOperation::Reverse_Subtract;
    } else if(sv == "min"sv || sv == "minimum"sv) {
        return BlendOperation::Min;
    } else if(sv == "max"sv || sv == "maximum"sv) {
        return BlendOperation::Max;
    } else {
        return BlendOperation::Add;
    }
}

[[nodiscard]] BlendOperation BlendOperationFromString(std::string str) noexcept;

[[nodiscard]] constexpr BlendColorWriteEnable BlendColorWriteEnableFromString(std::string_view sv) noexcept {
    if(sv.empty()) {
        return BlendColorWriteEnable::All;
    }

    BlendColorWriteEnable result = BlendColorWriteEnable::None;
    if(sv.find('r') != std::string_view::npos) {
        result |= BlendColorWriteEnable::Red;
    }
    if(sv.find('g') != std::string_view::npos) {
        result |= BlendColorWriteEnable::Green;
    }
    if(sv.find('b') != std::string_view::npos) {
        result |= BlendColorWriteEnable::Blue;
    }
    if(sv.find('a') != std::string_view::npos) {
        result |= BlendColorWriteEnable::Alpha;
    }
    return result;
}

[[nodiscard]] BlendColorWriteEnable BlendColorWriteEnableFromString(std::string str) noexcept;

[[nodiscard]] constexpr D3D11_FILL_MODE FillModeToD3DFillMode(const FillMode& fillmode) noexcept {
    switch(fillmode) {
    case FillMode::Solid: return D3D11_FILL_SOLID;
    case FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
    default: return D3D11_FILL_SOLID;
    }
}

[[nodiscard]] constexpr D3D11_CULL_MODE CullModeToD3DCullMode(const CullMode& cullmode) noexcept {
    switch(cullmode) {
    case CullMode::None: return D3D11_CULL_NONE;
    case CullMode::Front: return D3D11_CULL_FRONT;
    case CullMode::Back: return D3D11_CULL_BACK;
    default: return D3D11_CULL_BACK;
    }
}

[[nodiscard]] constexpr D3D11_RESOURCE_MISC_FLAG ResourceMiscFlagToD3DMiscFlag(const ResourceMiscFlag& flags) noexcept {
    return static_cast<D3D11_RESOURCE_MISC_FLAG>(flags);
}

[[nodiscard]] constexpr FillMode FillModeFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "solid"sv) {
        return FillMode::Solid;
    } else if(sv == "wire"sv || sv == "wireframe"sv) {
        return FillMode::Wireframe;
    } else {
        return FillMode::Solid;
    }
}

[[nodiscard]] FillMode FillModeFromString(std::string str) noexcept;

[[nodiscard]] constexpr CullMode CullModeFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "none"sv) {
        return CullMode::None;
    } else if(sv == "front"sv) {
        return CullMode::Front;
    } else if(sv == "back"sv) {
        return CullMode::Back;
    } else {
        return CullMode::Back;
    }
}

[[nodiscard]] CullMode CullModeFromString(std::string str) noexcept;

[[nodiscard]] constexpr WindingOrder WindingOrderFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv == "cw"sv || sv == "clockwise"sv) {
        return WindingOrder::CW;
    } else if(sv == "ccw"sv || sv == "counterclockwise"sv) {
        return WindingOrder::CCW;
    } else {
        return WindingOrder::CW;
    }
}

[[nodiscard]] WindingOrder WindingOrderFromString(std::string str) noexcept;

[[nodiscard]] std::string PipelineStageToString(const PipelineStage& stage) noexcept;

[[nodiscard]] constexpr PipelineStage PipelineStageFromString(std::string_view sv) noexcept {
    using namespace std::literals::string_view_literals;

    if(sv.empty()) {
        return PipelineStage::None;
    } else if(sv == "vs"sv) {
        return PipelineStage::Vs;
    } else if(sv == "hs"sv) {
        return PipelineStage::Hs;
    } else if(sv == "ds"sv) {
        return PipelineStage::Ds;
    } else if(sv == "gs"sv) {
        return PipelineStage::Gs;
    } else if(sv == "ps"sv) {
        return PipelineStage::Ps;
    } else if(sv == "cs"sv) {
        return PipelineStage::Cs;
    } else {
        return PipelineStage::None;
    }
}

[[nodiscard]] PipelineStage PipelineStageFromString(std::string stage) noexcept;