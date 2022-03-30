#include "Engine/Physics/ForceGenerator.hpp"

#include "Engine/Physics/RigidBody.hpp"

#include <algorithm>

void ForceGenerator::attach(RigidBody* body) noexcept {
    if(!is_attached(body)) {
        m_observers.push_back(body);
    }
}

void ForceGenerator::detach(RigidBody* body) noexcept {
    m_observers.erase(std::remove_if(std::begin(m_observers), std::end(m_observers), [body](const RigidBody* a) { return a == body; }), std::end(m_observers));
}

bool ForceGenerator::is_attached(const RigidBody* const body) const noexcept {
    return std::find(std::cbegin(m_observers), std::cend(m_observers), body) != std::cend(m_observers);
}

void ForceGenerator::detach_all() noexcept {
    m_observers.clear();
}
