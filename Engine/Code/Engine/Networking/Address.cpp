#include "Engine/Networking/Address.hpp"

namespace Net {

Address::Address(const std::string& /*value*/) noexcept {
    //TODO: Implement Address constructor from string
}

bool Address::operator!=(const Address& rhs) const noexcept {
    return !(*this == rhs);
}

bool Address::operator==(const Address& rhs) const noexcept {
    const auto compareIpv6 = [&]()-> const bool {
        return address.ipv6[0] == rhs.address.ipv6[0] &&
            address.ipv6[1] == rhs.address.ipv6[1] &&
            address.ipv6[2] == rhs.address.ipv6[2] &&
            address.ipv6[3] == rhs.address.ipv6[3];
    };
    return address.ipv4 == rhs.address.ipv4 && compareIpv6() && port == rhs.port;
}

} // namespace Net