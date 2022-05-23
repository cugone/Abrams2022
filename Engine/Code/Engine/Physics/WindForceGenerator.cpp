#include "Engine/Physics/WindForceGenerator.hpp"

#include "Engine/Physics/RigidBody.hpp"

void WindForceGenerator::notify(TimeUtils::FPSeconds deltaSeconds) const noexcept {
    for(auto* b : m_observers) {
        if(b) {
            if(const auto* collider = b->GetCollider(); collider != nullptr) {
                const auto A = collider->CalcArea();
                const auto& windVector = m_windSpeed.Get();
                const auto v2 = windVector.CalcLengthSquared();
                const auto p = m_airDensity;
                const auto F = 0.5f * p * v2 * A;
                b->ApplyForce(windVector.GetNormalize(), F, deltaSeconds);
            }
        }
    }
}

void WindForceGenerator::SetWindSpeed(const Velocity& newWindSpeed) noexcept {
    m_windSpeed = newWindSpeed;
}

void WindForceGenerator::SetAirResistance(float newAirResistance) noexcept {
    m_airResistance = newAirResistance;
}

void WindForceGenerator::SetDensity(float newAirDensity) noexcept {
    m_airDensity = newAirDensity;
}
