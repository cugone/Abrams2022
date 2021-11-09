#include "Engine/Physics/GravityForceGenerator.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/RigidBody.hpp"

GravityForceGenerator::GravityForceGenerator(const Vector2& gravity) noexcept
: ForceGenerator()
, g(gravity) {
    /* DO NOTHING */
}

void GravityForceGenerator::notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) const noexcept {
    if(g == Vector2::Zero) {
        return;
    }
    for(auto* body : _observers) {
        body->ApplyForce(g, deltaSeconds);
    }
}

void GravityForceGenerator::SetGravity(const Vector2& newGravity) noexcept {
    g = newGravity;
}
