#pragma once

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

        UUID(uint64_t uuid) noexcept;

        operator uint64_t() const noexcept { return m_UUID; }

    protected:
    private:
        uint64_t m_UUID{};
    };

} //namespace a2de


namespace std {
template<>
struct hash<a2de::UUID> {
    std::size_t operator()(const a2de::UUID& uuid) const noexcept {
        return hash<uint64_t>()(uuid);
    }
};
} // namespace std
