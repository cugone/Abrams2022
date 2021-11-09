#include "Engine/Core/UUID.hpp"

#include <random>

namespace a2de {

    UUID::UUID() noexcept {
        static thread_local std::random_device rd{};
        static thread_local std::mt19937_64 e{rd()};
        m_UUID = std::uniform_int_distribution<uint64_t>{}(e);
    }

    UUID::UUID(uint64_t uuid) noexcept
        : m_UUID{uuid}
    {
        /* DO NOTHING */
    }

} //namespace a2de
