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
        return m_address.ipv6[0] == rhs.m_address.ipv6[0] &&
            m_address.ipv6[1] == rhs.m_address.ipv6[1] &&
            m_address.ipv6[2] == rhs.m_address.ipv6[2] &&
            m_address.ipv6[3] == rhs.m_address.ipv6[3];
    };
    return m_address.ipv4 == rhs.m_address.ipv4 && compareIpv6() && m_port == rhs.m_port;
}

} // namespace Net