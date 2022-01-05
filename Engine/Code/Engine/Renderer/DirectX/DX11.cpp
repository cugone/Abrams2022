#include "Engine/Renderer/DirectX/DX11.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

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
//
//GraphicsCardDesc AdapterInfoToGraphicsCardDesc(const AdapterInfo& adapterInfo) noexcept {
//    GraphicsCardDesc desc{};
//    desc.Description = StringUtils::ConvertUnicodeToMultiByte(std::wstring(adapterInfo.desc.Description));
//    desc.DeviceId = adapterInfo.desc.DeviceId;
//    desc.VendorId = adapterInfo.desc.VendorId;
//    desc.SubSysId = adapterInfo.desc.SubSysId;
//    desc.Revision = adapterInfo.desc.Revision;
//    desc.DedicatedSystemMemory = adapterInfo.desc.DedicatedSystemMemory;
//    desc.DedicatedVideoMemory = adapterInfo.desc.DedicatedVideoMemory;
//    desc.SharedSystemMemory = adapterInfo.desc.SharedSystemMemory;
//    desc.is_software = (adapterInfo.desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
//    desc.is_unspecified = (adapterInfo.desc.Flags | DXGI_ADAPTER_FLAG3_NONE) == 0;
//    return desc;
//}

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
