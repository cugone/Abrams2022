#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include <vector>

class RigidBody;

class ForceGenerator {
public:
    ForceGenerator() noexcept = default;
    ForceGenerator(const ForceGenerator& other) = default;
    ForceGenerator(ForceGenerator&& other) = default;
    ForceGenerator& operator=(const ForceGenerator& other) = default;
    ForceGenerator& operator=(ForceGenerator&& other) = default;
    virtual ~ForceGenerator() noexcept = default;

    void attach(RigidBody* body) noexcept;
    void detach(RigidBody* body) noexcept;
    virtual void notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) const noexcept = 0;

    [[nodiscard]] bool is_attached(const RigidBody* const body) const noexcept;
    void detach_all() noexcept;

protected:
    std::vector<RigidBody*> _observers{};

private:
};
