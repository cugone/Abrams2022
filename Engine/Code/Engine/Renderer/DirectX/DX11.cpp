#include "Engine/Renderer/DirectX/DX11.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

constexpr const bitfield8_t MIP_MASK_BITS = 0b0000'0001;
constexpr const bitfield8_t MAG_MASK_BITS = 0b0000'0010;
constexpr const bitfield8_t MIN_MASK_BITS = 0b0000'0100;
constexpr const bitfield8_t COMPARISON_MASK_BITS = 0b0000'1000;
constexpr const bitfield8_t MINIMUM_MASK_BITS = 0b0001'0000;
constexpr const bitfield8_t MAXIMUM_MASK_BITS = 0b0010'0000;
constexpr const bitfield8_t ANISOTROPIC_MASK_BITS = 0b0100'0000;

//Dragons be here!! Look at your own risk!
D3D11_FILTER FilterModeToD3DFilter(const FilterMode& minFilterMode, const FilterMode& magFilterMode, const FilterMode& mipFilterMode, const FilterComparisonMode& minMaxComparison) noexcept {
    const auto filter_mask = GetFilterMaskFromModes(minFilterMode, magFilterMode, mipFilterMode, minMaxComparison);

    //Any anisotropic setting overrides all others.
    if((filter_mask & ANISOTROPIC_MASK_BITS) == ANISOTROPIC_MASK_BITS) {
        if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
            return D3D11_FILTER_COMPARISON_ANISOTROPIC;
        } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
            return D3D11_FILTER_MINIMUM_ANISOTROPIC;
        } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
            return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
        } else { //No comparison mode set
            return D3D11_FILTER_ANISOTROPIC;
        }
    } else { //Not anisotropic
        const auto isMip = (filter_mask & MIP_MASK_BITS) == MIP_MASK_BITS;
        const auto isMag = (filter_mask & MAG_MASK_BITS) == MAG_MASK_BITS;
        const auto isMin = (filter_mask & MIN_MASK_BITS) == MIN_MASK_BITS;
        if(!isMin && !isMag && !isMip) { //000
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
            } else {
                return D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
        } else if(!isMin && !isMag && isMip) { //001
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
            } else {
                return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            }
        } else if(!isMin && isMag && !isMip) { //010
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
            } else {
                return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            }
        } else if(!isMin && isMag && isMip) { //011
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
            } else {
                return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            }
        } else if(isMin && !isMag && !isMip) { //100
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
            } else {
                return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            }
        } else if(isMin && !isMag && isMip) { //101
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            } else {
                return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            }
        } else if(isMin && isMag && !isMip) { //110
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
            } else {
                return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            }
        } else if(isMin && isMag && isMip) { //111
            if((filter_mask & COMPARISON_MASK_BITS) == COMPARISON_MASK_BITS) {
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            } else if((filter_mask & MINIMUM_MASK_BITS) == MINIMUM_MASK_BITS) {
                return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
            } else if((filter_mask & MAXIMUM_MASK_BITS) == MAXIMUM_MASK_BITS) {
                return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
            } else {
                return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            }
        }
    }
    return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

DXGI_GPU_PREFERENCE AdapterPreferenceToDxgiGpuPreference(const AdapterPreference& preference) {
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

GraphicsCardDesc AdapterInfoToGraphicsCardDesc(const AdapterInfo& adapterInfo) noexcept {
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

//Dragons be here!! Look at your own risk!
bitfield8_t GetFilterMaskFromModes(const FilterMode& minFilterMode, const FilterMode& magFilterMode, const FilterMode& mipFilterMode, const FilterComparisonMode& minMaxComparison) noexcept {
    bitfield8_t result = 0;

    switch(minMaxComparison) {
    case FilterComparisonMode::None:
        result &= ~(COMPARISON_MASK_BITS | MINIMUM_MASK_BITS | MAXIMUM_MASK_BITS);
        break;
    case FilterComparisonMode::Minimum:
        result |= MINIMUM_MASK_BITS;
        break;
    case FilterComparisonMode::Maximum:
        result |= MAXIMUM_MASK_BITS;
        break;
    case FilterComparisonMode::Comparison:
        result |= COMPARISON_MASK_BITS;
        break;
    default:
        /* DO NOTHING */
        break;
    }

    if(mipFilterMode == FilterMode::Anisotropic) {
        result |= (ANISOTROPIC_MASK_BITS | MIP_MASK_BITS | MAG_MASK_BITS | MIN_MASK_BITS);
        return result; //Anisotropic overrides all other options.
    } else {
        if(mipFilterMode == FilterMode::Point) {
            result &= ~MIP_MASK_BITS;
        } else if(mipFilterMode == FilterMode::Linear) {
            result |= MIP_MASK_BITS;
        }
    }

    if(magFilterMode == FilterMode::Anisotropic) {
        result |= (ANISOTROPIC_MASK_BITS | MIP_MASK_BITS | MAG_MASK_BITS | MIN_MASK_BITS);
        return result; //Anisotropic overrides all other options.
    } else {
        if(magFilterMode == FilterMode::Point) {
            result &= ~MAG_MASK_BITS;
        } else if(magFilterMode == FilterMode::Linear) {
            result |= MAG_MASK_BITS;
        }
    }

    if(minFilterMode == FilterMode::Anisotropic) {
        result |= (ANISOTROPIC_MASK_BITS | MIP_MASK_BITS | MAG_MASK_BITS | MIN_MASK_BITS);
        return result; //Anisotropic overrides all other options.
    } else {
        if(minFilterMode == FilterMode::Point) {
            result &= ~MIN_MASK_BITS;
        } else if(minFilterMode == FilterMode::Linear) {
            result |= MIN_MASK_BITS;
        }
    }

    return result;
}

FilterMode FilterModeFromString(const char* str) noexcept {
    return FilterModeFromString(std::string(str ? str : ""));
}

FilterMode FilterModeFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "point" || str == "pt") {
        return FilterMode::Point;
    } else if(str == "linear" || str == "ln") {
        return FilterMode::Linear;
    } else if(str == "anisotropic" || str == "af") {
        return FilterMode::Anisotropic;
    } else {
        return FilterMode::Point;
    }
}

FilterComparisonMode FilterComparisonModeFromString(const char* str) noexcept {
    return FilterComparisonModeFromString(std::string(str ? str : ""));
}

FilterComparisonMode FilterComparisonModeFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "none") {
        return FilterComparisonMode::None;
    } else if(str == "min" || str == "minimum") {
        return FilterComparisonMode::Minimum;
    } else if(str == "max" || str == "maximum") {
        return FilterComparisonMode::Maximum;
    } else if(str == "comp" || str == "comparison") {
        return FilterComparisonMode::Comparison;
    } else {
        return FilterComparisonMode::None;
    }
}

D3D11_TEXTURE_ADDRESS_MODE AddressModeToD3DAddressMode(const TextureAddressMode& address_mode) noexcept {
    switch(address_mode) {
    case TextureAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
    case TextureAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
    case TextureAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TextureAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
    case TextureAddressMode::Mirror_Once: return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    default: return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}

TextureAddressMode TextureAddressModeFromString(const char* str) noexcept {
    return TextureAddressModeFromString(std::string(str ? str : ""));
}

TextureAddressMode TextureAddressModeFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "wrap") {
        return TextureAddressMode::Wrap;
    } else if(str == "mirror") {
        return TextureAddressMode::Mirror;
    } else if(str == "clamp") {
        return TextureAddressMode::Clamp;
    } else if(str == "border") {
        return TextureAddressMode::Border;
    } else if(str == "once" || str == "mirror_once") {
        return TextureAddressMode::Mirror_Once;
    } else {
        return TextureAddressMode::Wrap;
    }
}

D3D11_COMPARISON_FUNC ComparisonFunctionToD3DComparisonFunction(const ComparisonFunction& compareFunc) noexcept {
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

ComparisonFunction D3DComparisonFunctionToComparisonFunction(D3D11_COMPARISON_FUNC d3dcompareFunc) noexcept {
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

ComparisonFunction ComparisonFunctionFromString(const char* str) noexcept {
    return ComparisonFunctionFromString(std::string(str ? str : ""));
}

ComparisonFunction ComparisonFunctionFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "never" || str == "0") {
        return ComparisonFunction::Never;
    } else if(str == "less" || str == "<") {
        return ComparisonFunction::Less;
    } else if(str == "equal" || str == "==") {
        return ComparisonFunction::Equal;
    } else if(str == "lessequal" || str == "le" || str == "leq" || str == "<=") {
        return ComparisonFunction::Less_Equal;
    } else if(str == "greater" || str == ">") {
        return ComparisonFunction::Greater;
    } else if(str == "notequal" || str == "ne" || str == "neq" || str == "!=") {
        return ComparisonFunction::Not_Equal;
    } else if(str == "greaterequal" || str == "ge" || str == "geq" || str == ">=") {
        return ComparisonFunction::Greater_Equal;
    } else if(str == "always" || str == "1") {
        return ComparisonFunction::Always;
    } else {
        return ComparisonFunction::Never;
    }
}

D3D11_STENCIL_OP StencilOperationToD3DStencilOperation(const StencilOperation& stencil_operation) noexcept {
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

StencilOperation StencilOperationFromString(const char* str) noexcept {
    return StencilOperationFromString(std::string(str ? str : ""));
}

StencilOperation StencilOperationFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "keep") {
        return StencilOperation::Keep;
    } else if(str == "zero") {
        return StencilOperation::Zero;
    } else if(str == "replace") {
        return StencilOperation::Replace;
    } else if(str == "incrementandclamp" || str == "ic" || str == "incc" || str == "inc") {
        return StencilOperation::Increment_Clamp;
    } else if(str == "decrementandclamp" || str == "dc" || str == "decc" || str == "dec") {
        return StencilOperation::Decrement_Clamp;
    } else if(str == "invert" || str == "inv") {
        return StencilOperation::Invert;
    } else if(str == "incrementandwrap" || str == "iw" || str == "incw" || str == "inw") {
        return StencilOperation::Increment_Wrap;
    } else if(str == "decrementandwrap" || str == "dw" || str == "decw" || str == "dew") {
        return StencilOperation::Decrement_Wrap;
    } else {
        return StencilOperation::Keep;
    }
}

D3D11_USAGE BufferUsageToD3DUsage(const BufferUsage& usage) noexcept {
    switch(usage) {
    case BufferUsage::Default: return D3D11_USAGE_DEFAULT;
    case BufferUsage::Gpu: return D3D11_USAGE_DEFAULT;
    case BufferUsage::Static: return D3D11_USAGE_IMMUTABLE;
    case BufferUsage::Dynamic: return D3D11_USAGE_DYNAMIC;
    case BufferUsage::Staging: return D3D11_USAGE_STAGING;
    default: return D3D11_USAGE_DEFAULT;
    }
}

D3D11_BIND_FLAG BufferBindUsageToD3DBindFlags(const BufferBindUsage& bindFlags) noexcept {
    return static_cast<D3D11_BIND_FLAG>(bindFlags);
}

D3D11_CPU_ACCESS_FLAG CPUAccessFlagFromUsage(const BufferUsage& usage) noexcept {
    switch(usage) {
    case BufferUsage::Gpu: return D3D11_CPU_ACCESS_READ;
    case BufferUsage::Dynamic: return D3D11_CPU_ACCESS_WRITE;
    case BufferUsage::Static: return static_cast<D3D11_CPU_ACCESS_FLAG>(0U);
    case BufferUsage::Staging: return static_cast<D3D11_CPU_ACCESS_FLAG>(D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
    default: return static_cast<D3D11_CPU_ACCESS_FLAG>(0U);
    }
}

D3D11_PRIMITIVE_TOPOLOGY PrimitiveTypeToD3dTopology(const PrimitiveType& topology) noexcept {
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
DXGI_FORMAT ImageFormatToDxgiFormat(const ImageFormat& format) noexcept {
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

ImageFormat DxgiFormatToImageFormat(DXGI_FORMAT format) noexcept {
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

D3D11_BLEND BlendFactorToD3DBlendFactor(const BlendFactor& factor) noexcept {
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
    default: ERROR_AND_DIE("BlendFactor not defined.");
    }
}

D3D11_BLEND_OP BlendOpToD3DBlendOp(const BlendOperation& op) noexcept {
    switch(op) {
    case BlendOperation::Add: return D3D11_BLEND_OP_ADD;
    case BlendOperation::Subtract: return D3D11_BLEND_OP_SUBTRACT;
    case BlendOperation::Reverse_Subtract: return D3D11_BLEND_OP_REV_SUBTRACT;
    case BlendOperation::Min: return D3D11_BLEND_OP_MIN;
    case BlendOperation::Max: return D3D11_BLEND_OP_MAX;
    default: ERROR_AND_DIE("BlendOperation not defined.");
    }
}

UINT8 BlendColorWriteEnableToD3DBlendColorWriteEnable(const BlendColorWriteEnable& rt_mask) noexcept {
    return static_cast<UINT8>(rt_mask);
}

BlendFactor BlendFactorFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "zero") {
        return BlendFactor::Zero;
    } else if(str == "one") {
        return BlendFactor::One;
    } else if(str == "src_color") {
        return BlendFactor::Src_Color;
    } else if(str == "inv_src_color") {
        return BlendFactor::Inv_Src_Color;
    } else if(str == "src_alpha") {
        return BlendFactor::Src_Alpha;
    } else if(str == "inv_src_alpha") {
        return BlendFactor::Inv_Src_Alpha;
    } else if(str == "dest_alpha") {
        return BlendFactor::Dest_Alpha;
    } else if(str == "inv_dest_alpha") {
        return BlendFactor::Inv_Dest_Alpha;
    } else if(str == "dest_color") {
        return BlendFactor::Dest_Color;
    } else if(str == "inv_dest_color") {
        return BlendFactor::Inv_Dest_Color;
    } else if(str == "src_alpha_sat") {
        return BlendFactor::Src_Alpha_Sat;
    } else if(str == "blend_factor") {
        return BlendFactor::Blend_Factor;
    } else if(str == "inv_blend_factor") {
        return BlendFactor::Inv_Blend_Factor;
    } else if(str == "src1_color") {
        return BlendFactor::Src1_Color;
    } else if(str == "inv_src1_color") {
        return BlendFactor::Inv_Src1_Color;
    } else if(str == "src1_alpha") {
        return BlendFactor::Src1_Alpha;
    } else if(str == "inv_src1_alpha") {
        return BlendFactor::Inv_Src1_Alpha;
    } else {
        ERROR_AND_DIE("BlendFactor not defined.");
    }
}

BlendOperation BlendOperationFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "add") {
        return BlendOperation::Add;
    } else if(str == "subtract" || str == "sub") {
        return BlendOperation::Subtract;
    } else if(str == "rev_sub" || str == "rev_subtract" || str == "reverse_sub" || str == "reverse_subtract") {
        return BlendOperation::Reverse_Subtract;
    } else if(str == "min" || str == "minimum") {
        return BlendOperation::Min;
    } else if(str == "max" || str == "maximum") {
        return BlendOperation::Max;
    } else {
        ERROR_AND_DIE("BlendOperation not defined.");
    }
}

BlendColorWriteEnable BlendColorWriteEnableFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);

    if(str.empty()) {
        return BlendColorWriteEnable::All;
    }

    BlendColorWriteEnable result = BlendColorWriteEnable::None;
    if(str.find('r') != std::string::npos) {
        result |= BlendColorWriteEnable::Red;
    }
    if(str.find('g') != std::string::npos) {
        result |= BlendColorWriteEnable::Green;
    }
    if(str.find('b') != std::string::npos) {
        result |= BlendColorWriteEnable::Blue;
    }
    if(str.find('a') != std::string::npos) {
        result |= BlendColorWriteEnable::Alpha;
    }
    return result;
}

D3D11_FILL_MODE FillModeToD3DFillMode(const FillMode& fillmode) noexcept {
    switch(fillmode) {
    case FillMode::Solid: return D3D11_FILL_SOLID;
    case FillMode::Wireframe: return D3D11_FILL_WIREFRAME;
    default: return D3D11_FILL_SOLID;
    }
}

D3D11_CULL_MODE CullModeToD3DCullMode(const CullMode& cullmode) noexcept {
    switch(cullmode) {
    case CullMode::None: return D3D11_CULL_NONE;
    case CullMode::Front: return D3D11_CULL_FRONT;
    case CullMode::Back: return D3D11_CULL_BACK;
    default: return D3D11_CULL_BACK;
    }
}

FillMode FillModeFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "solid") {
        return FillMode::Solid;
    } else if(str == "wire" || str == "wireframe") {
        return FillMode::Wireframe;
    } else {
        return FillMode::Solid;
    }
}

CullMode CullModeFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "none") {
        return CullMode::None;
    } else if(str == "front") {
        return CullMode::Front;
    } else if(str == "back") {
        return CullMode::Back;
    } else {
        return CullMode::Back;
    }
}

WindingOrder WindingOrderFromString(std::string str) noexcept {
    str = StringUtils::ToLowerCase(str);
    if(str == "cw" || str == "clockwise") {
        return WindingOrder::CW;
    } else if(str == "ccw" || str == "counterclockwise") {
        return WindingOrder::CCW;
    } else {
        return WindingOrder::CW;
    }
}

D3D11_RESOURCE_MISC_FLAG ResourceMiscFlagToD3DMiscFlag(const ResourceMiscFlag& flags) noexcept {
    return static_cast<D3D11_RESOURCE_MISC_FLAG>(flags);
}

std::string PipelineStageToString(const PipelineStage& stage) noexcept {
    switch(stage) {
    case PipelineStage::None: return std::string{};
    case PipelineStage::Vs: return std::string{"vs"};
    case PipelineStage::Hs: return std::string{"hs"};
    case PipelineStage::Ds: return std::string{"ds"};
    case PipelineStage::Gs: return std::string{"gs"};
    case PipelineStage::Ps: return std::string{"ps"};
    case PipelineStage::Cs: return std::string{"cs"};
    default: return std::string{""};
    }
}

PipelineStage PipelineStageFromString(std::string stage) noexcept {
    stage = StringUtils::ToLowerCase(stage);
    if(stage.empty())
        return PipelineStage::None;
    else if(stage == std::string{"vs"})
        return PipelineStage::Vs;
    else if(stage == std::string{"hs"})
        return PipelineStage::Hs;
    else if(stage == std::string{"ds"})
        return PipelineStage::Ds;
    else if(stage == std::string{"gs"})
        return PipelineStage::Gs;
    else if(stage == std::string{"ps"})
        return PipelineStage::Ps;
    else if(stage == std::string{"cs"})
        return PipelineStage::Cs;
    else
        return PipelineStage::None;
}
