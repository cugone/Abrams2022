#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Joint.hpp"

class Renderer;

struct SpringJointDef : public JointDef {
    SpringJointDef() = default;
    virtual ~SpringJointDef() = default;
    float length = 1.0f;
    float k = 1.0f;
};

class SpringJoint : public Joint {
public:
    SpringJoint() = delete;
    explicit SpringJoint(const SpringJointDef& def) noexcept;
    SpringJoint(const SpringJoint& other) = default;
    SpringJoint(SpringJoint&& other) = default;
    SpringJoint& operator=(const SpringJoint& other) = default;
    SpringJoint& operator=(SpringJoint&& other) = default;
    virtual ~SpringJoint() = default;

    void Notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void DebugRender() const noexcept override;

    [[nodiscard]] bool IsNotAttached() const noexcept override;
    void Attach(RigidBody* a, RigidBody* b, Vector2 localAnchorA = Vector2::Zero, Vector2 localAnchorB = Vector2::Zero) noexcept override;
    void Detach(const RigidBody* body) noexcept override;
    void DetachAll() noexcept override;

    [[nodiscard]] RigidBody* GetBodyA() const noexcept override;
    [[nodiscard]] RigidBody* GetBodyB() const noexcept override;

    [[nodiscard]] Vector2 GetAnchorA() const noexcept override;
    [[nodiscard]] Vector2 GetAnchorB() const noexcept override;

    [[nodiscard]] float GetMassA() const noexcept override;
    [[nodiscard]] float GetMassB() const noexcept override;

protected:
private:
    SpringJointDef _def{};

    [[nodiscard]] bool ConstraintViolated() const noexcept override;
    void SolvePositionConstraint() const noexcept override;
    void SolveVelocityConstraint() const noexcept override;

    friend class PhysicsSystem;
};
