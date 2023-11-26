#include "Engine/RHI/RHITypes.hpp"

#include "Engine/Math/MathUtils.hpp"

#include <iomanip>
#include <format>
#include <sstream>
#include <type_traits>

bool operator==(const ViewportDesc& a, const ViewportDesc& b) noexcept {
    const bool x = MathUtils::IsEquivalent(a.x, b.x);
    const bool y = MathUtils::IsEquivalent(a.y, b.y);
    const bool w = MathUtils::IsEquivalent(a.width, b.width);
    const bool h = MathUtils::IsEquivalent(a.height, b.height);
    const bool i = MathUtils::IsEquivalent(a.minDepth, b.minDepth);
    const bool m = MathUtils::IsEquivalent(a.maxDepth, b.maxDepth);
    return x && y && w && h && i && m;
}

bool operator!=(const ViewportDesc& a, const ViewportDesc& b) noexcept {
    return !(a == b);
}

bool DisplayDescLTComparator::operator()(const DisplayDesc& a, const DisplayDesc& b) const noexcept {
    if(a.width < b.width) {
        return true;
    }
    if(b.width < a.width) {
        return false;
    }
    if(a.height < b.height) {
        return true;
    }
    if(b.height < a.height) {
        return false;
    }
    if(a.refreshRateHz < b.refreshRateHz) {
        return true;
    }
    if(b.refreshRateHz < a.refreshRateHz) {
        return false;
    }
    return false;
}

bool DisplayDescGTComparator::operator()(const DisplayDesc& a, const DisplayDesc& b) const noexcept {
    return DisplayDescLTComparator{}.operator()(b, a);
}

RHIOutputMode& operator++(RHIOutputMode& mode) noexcept {
    using underlying = std::underlying_type_t<RHIOutputMode>;
    mode = static_cast<RHIOutputMode>(static_cast<underlying>(mode) + 1);
    if(mode == RHIOutputMode::Last_) {
        mode = RHIOutputMode::First_;
    }
    return mode;
}

RHIOutputMode operator++(RHIOutputMode& mode, int) noexcept {
    RHIOutputMode result = mode;
    ++mode;
    return result;
}

std::string_view VendorIdToFriendlyName(unsigned int vendorId) noexcept {
    switch(vendorId) {
    case 0x0014: return "Loongson Technology LLC";
    case 0x0731: return "Jingjia Microelectronics Co Ltd";
    case 0x0955: return "NVIDIA Corp.";
    case 0x1002: return "Advanced Micro Devices, Inc. [AMD/ATI]";
    case 0x1022: return "Advanced Micro Devices, Inc. [AMD]";
    case 0x1043: return "ASUSTek Computer Inc.";
    case 0x10DE: return "NVIDIA Corporation";
    case 0x12D2: return "NVidia / SGS Thomson(Joint Venture)";
    case 0x1414: return "Microsoft Corporation";
    case 0x1AF4: return "Red Hat, Inc.";
    case 0x1D17: return "Zhaoxin";
    case 0x1DB7: return "Phytium Technology Co., Ltd.";
    case 0x8086: return "Intel Corporation";
    case 0xBDBD: return "Blackmagic Design";
    case 0x1000: return "LSI Logic / Symbios Logic";
    case 0x100B: return "National Semiconductor Corporation";
    case 0x1011: return "Digital Equipment Corporation";
    case 0x1014: return "IBM";
    case 0x1025: return "Acer Incorporated [ALI]";
    case 0x102B: return "Matrox Electronics Systems Ltd.";
    case 0x104C: return "Texas Instruments";
    case 0x1067: return "Mitsubishi Electric";
    case 0x106C: return "Hynix Semiconductor";
    case 0x1073: return "Yamaha Corporation";
    case 0x1091: return "Intergraph Corporation";
    case 0x10A9: return "Silicon Graphics Intl. Corp.";
    case 0x10BA: return "Mitsubishi Electric Corp.";
    case 0x10CF: return "Fujitsu Limited.";
    case 0x10EA: return "Intergraphics";
    case 0x10EB: return "Artists Graphics";
    case 0x1106: return "VIA Technologies, Inc.";
    case 0x1139: return "Dynamic Pictures, Inc.";
    case 0x17F3: return "RDC Semiconductor, Inc.";
    case 0x18CA: return "XGI Technology Inc. (eXtreme Graphics Innovation)";
    case 0x18EA: return "Matrox Graphics, Inc.";
    case 0x1A03: return "ASPEED Technology, Inc.";
    case 0x3D3D: return "3DLabs";
    case 0x5333: return "S3 Graphics Ltd.";
    case 0x5853: return "XenSource, Inc.";
    case 0x80EE: return "InnoTek Systemberatung GmbH";
    default:     return "Unknown";
    }
}

std::string StringUtils::to_string(const GraphicsCardDesc& graphicsCardDesc) noexcept {
    std::ostringstream ss;
    const auto videoMemAsGB = static_cast<long double>(graphicsCardDesc.DedicatedVideoMemory) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den;
    const auto systemMemAsGB = static_cast<long double>(graphicsCardDesc.DedicatedSystemMemory) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den;
    const auto sharedMemAsGB = static_cast<long double>(graphicsCardDesc.SharedSystemMemory) * MathUtils::GIB_BYTES_RATIO.num / MathUtils::GIB_BYTES_RATIO.den;
    ss << std::left;
    ss << std::setw(40);
    ss << "Name:";
    ss << std::right;
    ss << std::setw(35);
    ss << graphicsCardDesc.Description;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Vendor ID:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::hex;
    ss << graphicsCardDesc.VendorId;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Vendor ID:";
    ss << std::right;
    ss << std::setw(35);
    ss << graphicsCardDesc.VendorId;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Vendor Name:";
    ss << std::right;
    ss << std::setw(35);
    ss << VendorIdToFriendlyName(graphicsCardDesc.VendorId);
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Device ID:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::hex;
    ss << graphicsCardDesc.DeviceId;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Subsystem ID:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::hex;
    ss << graphicsCardDesc.SubSysId;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Revision:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::hex;
    ss << graphicsCardDesc.Revision;
    ss << '\n';
    ss << std::left;
    ss << std::setw(40);
    ss << "Video Memory:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::hex;
    ss << videoMemAsGB;
    ss << " GB\n";
    ss << std::left;
    ss << std::setw(40);
    ss << "System Memory:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::setprecision(1);
    ss << systemMemAsGB;
    ss << " GB\n";
    ss << std::left;
    ss << std::setw(40);
    ss << "Shared System Memory:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::setprecision(1);
    ss << sharedMemAsGB;
    ss << " GB\n";
    ss << std::left;
    ss << std::setw(40);
    ss << "Adapter Type:";
    ss << std::right;
    ss << std::setw(35);
    ss << std::string{(graphicsCardDesc.is_unspecified ? "Unknown" : (graphicsCardDesc.is_software ? "Software" : "Hardware"))};
    return ss.str();
}
