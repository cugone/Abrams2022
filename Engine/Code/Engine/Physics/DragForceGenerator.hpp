#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/ForceGenerator.hpp"

class DragForceGenerator : public ForceGenerator {
public:
    DragForceGenerator() = default;
    explicit DragForceGenerator(const Vector2& k1k2) noexcept;
    DragForceGenerator(const DragForceGenerator& other) = default;
    DragForceGenerator(DragForceGenerator&& other) = default;
    DragForceGenerator& operator=(const DragForceGenerator& other) = default;
    DragForceGenerator& operator=(DragForceGenerator&& other) = default;
    virtual ~DragForceGenerator() = default;

    void notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) const noexcept override;

    void SetCoefficients(const Vector2& k1k2) noexcept;

protected:
private:
    Vector2 _k1k2{};
};
