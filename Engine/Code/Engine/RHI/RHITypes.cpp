#include "Engine/RHI/RHITypes.hpp"

#include "Engine/Math/MathUtils.hpp"

#include <iomanip>
#include <format>
#include <type_traits>

bool operator==(const ViewportDesc& a, const ViewportDesc& b) noexcept {
    bool x = MathUtils::IsEquivalent(a.x, b.x);
    bool y = MathUtils::IsEquivalent(a.y, b.y);
    bool w = MathUtils::IsEquivalent(a.width, b.width);
    bool h = MathUtils::IsEquivalent(a.height, b.height);
    bool i = MathUtils::IsEquivalent(a.minDepth, b.minDepth);
    bool m = MathUtils::IsEquivalent(a.maxDepth, b.maxDepth);
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
    const auto idAsHex = std::format("{:X}", vendorId);
    if(idAsHex == "0014") {
        return "Loongson Technology LLC";
    } else if(idAsHex == "0731") {
        return "Jingjia Microelectronics Co Ltd";
    } else if(idAsHex == "0955") {
        return "NVIDIA Corp.";
    } else if(idAsHex == "1002") {
        return "Advanced Micro Devices, Inc. [AMD/ATI]";
    } else if(idAsHex == "1022") {
        return "Advanced Micro Devices, Inc. [AMD]";
    } else if(idAsHex == "1043") {
        return "ASUSTek Computer Inc.";
    } else if(idAsHex == "10DE") {
        return "NVIDIA Corporation";
    } else if(idAsHex == "12D2") {
        return "NVidia / SGS Thomson(Joint Venture)";
    } else if(idAsHex == "1414") {
        return "Microsoft Corporation";
    } else if(idAsHex == "1AF4") {
        return "Red Hat, Inc.";
    } else if(idAsHex == "1D17") {
        return "Zhaoxin";
    } else if(idAsHex == "1DB7") {
        return "Phytium Technology Co., Ltd.";
    } else if(idAsHex == "8086") {
        return "Intel Corporation";
    } else if(idAsHex == "BDBD") {
        return "Blackmagic Design";
    } else if(idAsHex == "1000") {
        return "LSI Logic / Symbios Logic";
    } else if(idAsHex == "100B") {
        return "National Semiconductor Corporation";
    } else if(idAsHex == "1011") {
        return "Digital Equipment Corporation";
    } else if(idAsHex == "1014") {
        return "IBM";
    } else if(idAsHex == "1025") {
        return "Acer Incorporated [ALI]";
    } else if(idAsHex == "102B") {
        return "Matrox Electronics Systems Ltd.";
    } else if(idAsHex == "104C") {
        return "Texas Instruments";
    } else if(idAsHex == "1067") {
        return "Mitsubishi Electric";
    } else if(idAsHex == "106C") {
        return "Hynix Semiconductor";
    } else if(idAsHex == "1073") {
        return "Yamaha Corporation";
    } else if(idAsHex == "1091") {
        return "Intergraph Corporation";
    } else if(idAsHex == "10A9") {
        return "Silicon Graphics Intl. Corp.";
    } else if(idAsHex == "10BA") {
        return "Mitsubishi Electric Corp.";
    } else if(idAsHex == "10CF") {
        return "Fujitsu Limited.";
    } else if(idAsHex == "10EA") {
        return "Intergraphics";
    } else if(idAsHex == "10EB") {
        return "Artists Graphics";
    } else if(idAsHex == "1106") {
        return "VIA Technologies, Inc.";
    } else if(idAsHex == "1139") {
        return "Dynamic Pictures, Inc.";
    } else if(idAsHex == "17F3") {
        return "RDC Semiconductor, Inc.";
    } else if(idAsHex == "18CA") {
        return "XGI Technology Inc. (eXtreme Graphics Innovation)";
    } else if(idAsHex == "18EA") {
        return "Matrox Graphics, Inc.";
    } else if(idAsHex == "1A03") {
        return "ASPEED Technology, Inc.";
    } else if(idAsHex == "3D3D") {
        return "3DLabs";
    } else if(idAsHex == "5333") {
        return "S3 Graphics Ltd.";
    } else if(idAsHex == "5853") {
        return "XenSource, Inc.";
    } else if(idAsHex == "80EE") {
        return "InnoTek Systemberatung GmbH";
    } else if(idAsHex == "HUT") {
        return "0c Consumer";
    } else {
        return "Unknown";
    }
}
