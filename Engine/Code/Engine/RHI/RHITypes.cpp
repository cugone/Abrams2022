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
    if(idAsHex == "0955") {
        return "NVIDIA Corp.";
    } else if(idAsHex == "10DE") {
        return "NVIDIA Corporation";
    } else if(idAsHex == "12D2") {
        return "NVidia / SGS Thomson(Joint Venture)";
    } else {
        return "Unknown";
    }
}
