#include "Engine/Physics/ForceGenerator.hpp"

#include "Engine/Physics/RigidBody.hpp"

#include <algorithm>

void ForceGenerator::attach(RigidBody* body) noexcept {
    if(!is_attached(body)) {
        _observers.push_back(body);
    }
}

void ForceGenerator::detach(RigidBody* body) noexcept {
    _observers.erase(std::remove_if(std::begin(_observers), std::end(_observers), [body](const RigidBody* a) { return a == body; }), std::end(_observers));
}

bool ForceGenerator::is_attached(const RigidBody* const body) const noexcept {
    return std::find(std::cbegin(_observers), std::cend(_observers), body) != std::cend(_observers);
}

void ForceGenerator::detach_all() noexcept {
    _observers.clear();
}
