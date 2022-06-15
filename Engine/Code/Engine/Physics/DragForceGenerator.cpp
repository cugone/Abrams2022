#include "Engine/Physics/DragForceGenerator.hpp"

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Physics/RigidBody.hpp"

DragForceGenerator::DragForceGenerator(const Vector2& k1k2) noexcept
: ForceGenerator()
, m_k1k2{k1k2} {
    /* DO NOTHING */
}

void DragForceGenerator::notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) const noexcept {
    for(auto* body : m_observers) {
        if(auto dragForce = body->GetVelocity(); !MathUtils::IsEquivalentToZero(dragForce)) {
            auto dragCoeff = dragForce.CalcLength();
            dragCoeff = m_k1k2.x * dragCoeff + m_k1k2.y * dragCoeff * dragCoeff;
            dragForce.Normalize();
            dragForce *= -dragCoeff;
            body->ApplyForce(dragForce, deltaSeconds);
        }
    }
}

void DragForceGenerator::SetCoefficients(const Vector2& k1k2) noexcept {
    m_k1k2 = k1k2;
}
