#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/Collider.hpp"
#include "Engine/Physics/PhysicsTypes.hpp"

#include <memory>

struct RigidBodyDesc {
    Position initialPosition{};
    Velocity initialVelocity{};
    Acceleration initialAcceleration{};
    Collider* collider{};
    PhysicsMaterial physicsMaterial{};
    PhysicsDesc physicsDesc{};
    RigidBodyDesc() noexcept {
        /* DO NOTHING */
    }
    RigidBodyDesc(Position initialPos, Velocity initialVel, Acceleration initialAcc, Collider* coll, const PhysicsMaterial& physMat, const PhysicsDesc& physDesc) noexcept
    : initialPosition{initialPos}
    , initialVelocity{initialVel}
    , initialAcceleration{initialAcc}
    , collider{coll}
    , physicsMaterial{physMat}
    , physicsDesc{physDesc} {
        /* DO NOTHING */
    }
    RigidBodyDesc(RigidBodyDesc&& other) noexcept
    : initialPosition(std::move(other.initialPosition))
    , initialVelocity(std::move(other.initialVelocity))
    , initialAcceleration(std::move(other.initialAcceleration))
    , physicsMaterial(std::move(other.physicsMaterial))
    , physicsDesc(std::move(other.physicsDesc)) {
        collider = std::move(other.collider);
        other.collider = nullptr;
    }
    RigidBodyDesc& operator=(RigidBodyDesc&& other) noexcept {
        initialPosition = std::move(other.initialPosition);
        initialVelocity = std::move(other.initialVelocity);
        initialAcceleration = std::move(other.initialAcceleration);
        physicsMaterial = std::move(other.physicsMaterial);
        physicsDesc = std::move(other.physicsDesc);
        collider = std::move(other.collider);
        other.collider = nullptr;
        return *this;
    }

    RigidBodyDesc(const RigidBodyDesc& other) noexcept
    : initialPosition(other.initialPosition)
    , initialVelocity(other.initialVelocity)
    , initialAcceleration(other.initialAcceleration)
    , physicsMaterial(other.physicsMaterial)
    , physicsDesc(other.physicsDesc) {
        auto* new_collider = other.collider->Clone();
        delete collider;
        collider = new_collider;
    }
    RigidBodyDesc& operator=(const RigidBodyDesc& other) noexcept {
        if(&other == this) {
            return *this;
        }
        initialPosition = other.initialPosition;
        initialVelocity = other.initialVelocity;
        initialAcceleration = other.initialAcceleration;
        physicsMaterial = other.physicsMaterial;
        physicsDesc = other.physicsDesc;
        auto* new_collider = other.collider->Clone();
        delete collider;
        collider = new_collider;
        return *this;
    }
    ~RigidBodyDesc() noexcept {
        delete collider;
        collider = nullptr;
    }
};

struct PhysicsSystemDesc;
class PhysicsSystem;

class RigidBody {
public:
    explicit RigidBody(const RigidBodyDesc& desc = RigidBodyDesc{});

    RigidBody() = delete;
    RigidBody(RigidBody&& other) noexcept = default;
    RigidBody& operator=(RigidBody&& rhs) noexcept = default;

    RigidBody(const RigidBody& other) noexcept = default;
    RigidBody& operator=(const RigidBody& rhs) noexcept = default;
    ~RigidBody() noexcept = default;

    Matrix4 transform{};

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void DebugRender() const;
    void Endframe();

    void EnablePhysics(bool enabled);
    void EnableGravity(bool enabled);
    void EnableDrag(bool enabled);
    [[nodiscard]] bool IsPhysicsEnabled() const;
    [[nodiscard]] bool IsGravityEnabled() const;
    [[nodiscard]] bool IsDragEnabled() const;
    [[nodiscard]] bool IsDynamic() const noexcept;

    void SetAwake(bool awake) noexcept;
    void Wake() noexcept;
    void Sleep() noexcept;
    [[nodiscard]] bool IsAwake() const;

    [[nodiscard]] float GetMass() const;
    [[nodiscard]] float GetInverseMass() const;

    [[nodiscard]] Matrix4 GetParentTransform() const;

    [[nodiscard]] Vector2 CalcForceVector() noexcept;

    void ApplyImpulse(const Vector2& impulse);
    void ApplyImpulse(const Vector2& direction, float magnitude);

    void ApplyForce(const Vector2& force, const TimeUtils::FPSeconds& duration);
    void ApplyForce(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration);

    void ApplyTorque(float force, const TimeUtils::FPSeconds& duration);
    void ApplyTorque(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration);

    void ApplyTorqueAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration);
    void ApplyTorqueAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration);

    void ApplyForceAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration);
    void ApplyForceAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration);

    void ApplyImpulseAt(const Vector2& position_on_object, const Vector2& direction, float magnitude);
    void ApplyImpulseAt(const Vector2& position_on_object, const Vector2& force);

    [[nodiscard]] const OBB2 GetBounds() const;

    void SetPosition(const Vector2& newPosition, bool teleport = false) noexcept;
    [[nodiscard]] const Vector2& GetPosition() const;

    void SetVelocity(const Vector2& newVelocity) noexcept;
    [[nodiscard]] const Vector2& GetVelocity() const;

    [[nodiscard]] const Vector2& GetAcceleration() const;
    [[nodiscard]] Vector2 CalcDimensions() const;
    [[nodiscard]] float GetOrientationDegrees() const;
    [[nodiscard]] float GetAngularVelocityDegrees() const;
    [[nodiscard]] float GetAngularAccelerationDegrees() const;

    [[nodiscard]] const Collider* GetCollider() const noexcept;
    [[nodiscard]] Collider* GetCollider() noexcept;

    void FellOutOfWorld() noexcept;
    [[nodiscard]] const bool ShouldKill() const noexcept;
    [[nodiscard]] const bool IsRotationLocked() const noexcept;
    void LockRotation(bool shouldLockRotation) noexcept;

protected:
private:
    void SetAcceleration(const Vector2& newAccleration) noexcept;
    void Integrate(TimeUtils::FPSeconds deltaSeconds) noexcept;

    RigidBodyDesc m_rigidbodyDesc{};
    RigidBody* m_parent = nullptr;
    std::vector<RigidBody*> m_children{};
    Vector2 m_position{};
    Vector2 m_velocity{};
    Vector2 m_acceleration{};
    float m_prev_orientationDegrees = 0.0f;
    float m_orientationDegrees = 0.0f;
    float m_angular_acceleration = 0.0f;
    TimeUtils::FPSeconds m_dt{};
    TimeUtils::FPSeconds m_time_since_last_move{};
    std::vector<std::pair<Vector2, TimeUtils::FPSeconds>> m_linear_forces{};
    std::vector<Vector2> m_linear_impulses{};
    std::vector<std::pair<float, TimeUtils::FPSeconds>> m_angular_forces{};
    std::vector<float> m_angular_impulses{};
    bool m_is_colliding = false;
    bool m_is_awake = true;
    bool m_should_kill = false;
    bool m_should_lock_rotation = false;

    friend class PhysicsSystem;
};
