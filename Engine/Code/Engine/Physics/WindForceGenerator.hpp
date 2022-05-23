#pragma once

#include "Engine/Physics/ForceGenerator.hpp"
#include "Engine/Physics/PhysicsTypes.hpp"

class WindForceGenerator : public ForceGenerator {
public:
    WindForceGenerator() = default;
    WindForceGenerator(const WindForceGenerator& other) = default;
    WindForceGenerator(WindForceGenerator&& other) = default;
    WindForceGenerator& operator=(const WindForceGenerator& other) = default;
    WindForceGenerator& operator=(WindForceGenerator&& other) = default;
    virtual ~WindForceGenerator() = default;

    void notify(TimeUtils::FPSeconds deltaSeconds) const noexcept override;

    void SetWindSpeed(const Velocity& newWindSpeed) noexcept;
    void SetAirResistance(float newAirResistance) noexcept;
    void SetDensity(float newAirDensity) noexcept;

protected:
private:
    Velocity m_windSpeed{};
    float m_airResistance{0.0f};
    float m_airDensity{1.229f}; //Defaults to average at sea level of Earth
};
