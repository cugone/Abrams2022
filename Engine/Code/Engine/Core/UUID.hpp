#pragma once

#include <cstdint>
#include <functional>

namespace a2de {

    class UUID {
    public:
        UUID() noexcept;
        UUID(const UUID& other) noexcept = default;
        UUID(UUID&& other) noexcept = default;
        UUID& operator=(const UUID& other) noexcept = default;
        UUID& operator=(UUID&& other) noexcept = default;
        ~UUID() noexcept = default;

        UUID(std::uint64_t uuid) noexcept;

        operator std::uint64_t() const noexcept { return m_UUID; }
        operator std::uint64_t() noexcept { return m_UUID; };

    protected:
    private:
        std::uint64_t m_UUID{};
    };

} //namespace a2de


namespace std {

template<typename T> struct hash;

template<>
struct hash<a2de::UUID> {
    std::size_t operator()(const a2de::UUID& uuid) const noexcept {
        return static_cast<std::size_t>(static_cast<std::uint64_t>(uuid));
    }
};
} // namespace std
