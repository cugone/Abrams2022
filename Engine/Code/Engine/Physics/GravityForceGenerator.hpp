#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/ForceGenerator.hpp"

class GravityForceGenerator : public ForceGenerator {
public:
    GravityForceGenerator() noexcept = default;
    explicit GravityForceGenerator(const Vector2& gravity) noexcept;
    GravityForceGenerator(const GravityForceGenerator& other) noexcept = default;
    GravityForceGenerator(GravityForceGenerator&& other) noexcept = default;
    GravityForceGenerator& operator=(const GravityForceGenerator& other) noexcept = default;
    GravityForceGenerator& operator=(GravityForceGenerator&& other) noexcept = default;
    virtual ~GravityForceGenerator() noexcept = default;
    void notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) const noexcept override;

    void SetGravity(const Vector2& newGravity) noexcept;

protected:
private:
    Vector2 g{0.0f, 10.0f};
};
