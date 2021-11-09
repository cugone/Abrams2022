#include "Engine/Physics/RigidBody.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Physics/PhysicsSystem.hpp"
#include "Engine/Physics/PhysicsUtils.hpp"
#include "Engine/Profiling/ProfileLogScope.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Services/IPhysicsService.hpp"

#include <cmath>
#include <numeric>
#include <type_traits>

RigidBody::RigidBody(const RigidBodyDesc& desc /*= RigidBodyDesc{}*/)
: rigidbodyDesc(desc)
, velocity(rigidbodyDesc.initialVelocity)
, position(rigidbodyDesc.initialPosition)
, acceleration(rigidbodyDesc.initialAcceleration) {
    const auto area = rigidbodyDesc.collider->CalcArea();
    if(MathUtils::IsEquivalentToZero(rigidbodyDesc.physicsMaterial.density) || MathUtils::IsEquivalentToZero(area)) {
        rigidbodyDesc.physicsDesc.mass = 0.0f;
    } else {
        rigidbodyDesc.physicsDesc.mass = rigidbodyDesc.physicsMaterial.density * area;
        rigidbodyDesc.physicsDesc.mass = std::pow(rigidbodyDesc.physicsDesc.mass, rigidbodyDesc.physicsMaterial.massExponent);
        if(!MathUtils::IsEquivalentToZero(rigidbodyDesc.physicsDesc.mass) && rigidbodyDesc.physicsDesc.mass < 0.001f) {
            rigidbodyDesc.physicsDesc.mass = 0.001f;
        }
    }
}

void RigidBody::BeginFrame() {
    static constexpr auto pred = [](const auto& force) { return force.second.count() <= 0.0f; };
    if(!linear_forces.empty()) {
        linear_forces.erase(std::remove_if(linear_forces.begin(), linear_forces.end(), pred), linear_forces.end());
    }
    if(!angular_forces.empty()) {
        angular_forces.erase(std::remove_if(angular_forces.begin(), angular_forces.end(), pred), angular_forces.end());
    }
}

void RigidBody::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(!IsPhysicsEnabled() || !IsDynamic() || !IsAwake() || MathUtils::IsEquivalentToZero(GetInverseMass())) {
        linear_impulses.clear();
        angular_impulses.clear();
        linear_forces.clear();
        angular_forces.clear();
        return;
    }
    const auto inv_mass = GetInverseMass();
    const auto linear_impulse_sum = std::accumulate(std::begin(linear_impulses), std::end(linear_impulses), Vector2::Zero);
    const auto angular_impulse_sum = std::accumulate(std::begin(angular_impulses), std::end(angular_impulses), 0.0f);
    linear_impulses.clear();
    angular_impulses.clear();

    using LinearForceType = typename std::decay<decltype(*linear_forces.begin())>::type;
    const auto linear_acc = [](const LinearForceType& a, const LinearForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto linear_force_sum = std::accumulate(std::begin(linear_forces), std::end(linear_forces), std::make_pair(Vector2::Zero, TimeUtils::FPSeconds::zero()), linear_acc);

    using AngularForceType = typename std::decay<decltype(*angular_forces.begin())>::type;
    const auto angular_acc = [](const AngularForceType& a, const AngularForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto angular_force_sum = std::accumulate(std::begin(angular_forces), std::end(angular_forces), std::make_pair(0.0f, TimeUtils::FPSeconds::zero()), angular_acc);

    //https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet

    const auto new_angular_acceleration = (angular_impulse_sum + angular_force_sum.first) * inv_mass;
    const auto t = deltaSeconds.count();
    dt = deltaSeconds;

    auto new_position = GetPosition() + GetVelocity() * dt.count() + GetAcceleration() * (dt.count() * dt.count() * 0.5f);
    auto new_acceleration = (linear_impulse_sum + linear_force_sum.first) * inv_mass;
    auto new_velocity = GetVelocity() + (GetAcceleration() + new_acceleration) * (dt.count() * 0.5f);
    new_velocity *= rigidbodyDesc.physicsDesc.linearDamping;

    {
        const bool is_near_zero = MathUtils::IsEquivalentToZero(new_position);
        const bool is_inf = (std::isinf(new_position.x) || std::isinf(new_position.y));
        const bool is_nan = (std::isnan(new_position.x) || std::isnan(new_position.y));
        const bool should_clamp = is_near_zero || is_nan || is_inf;
        if(should_clamp) {
            new_position = Vector2::Zero;
        }
    }
    {
        const bool is_near_zero = MathUtils::IsEquivalentToZero(new_acceleration);
        const bool is_inf = (std::isinf(new_acceleration.x) || std::isinf(new_acceleration.y));
        const bool is_nan = (std::isnan(new_acceleration.x) || std::isnan(new_acceleration.y));
        const bool should_clamp = is_near_zero || is_nan || is_inf;
        if(should_clamp) {
            new_acceleration = Vector2::Zero;
        }
    }
    {
        const bool is_near_zero = MathUtils::IsEquivalentToZero(new_velocity);
        const bool is_inf = (std::isinf(new_velocity.x) || std::isinf(new_velocity.y));
        const bool is_nan = (std::isnan(new_velocity.x) || std::isnan(new_velocity.y));
        const bool should_clamp = is_near_zero || is_nan || is_inf;
        if(should_clamp) {
            new_velocity = Vector2::Zero;
        }
    }

    auto deltaPosition = position - new_position;
    auto deltaOrientation = orientationDegrees - prev_orientationDegrees;
    if(MathUtils::IsEquivalentToZero(deltaPosition)
       && MathUtils::IsEquivalentToZero(deltaOrientation)) {
        time_since_last_move += dt;
    } else {
        time_since_last_move = TimeUtils::FPSeconds{0.0f};
    }
    is_awake = time_since_last_move < TimeUtils::FPSeconds{1.0f};

    SetPosition(new_position);
    SetVelocity(new_velocity);
    SetAcceleration(new_acceleration);

    const auto& maxAngularSpeed = rigidbodyDesc.physicsDesc.maxAngularSpeed;
    auto new_angular_velocity = std::clamp((2.0f * orientationDegrees - prev_orientationDegrees) / dt.count(), -maxAngularSpeed, maxAngularSpeed);
    new_angular_velocity *= rigidbodyDesc.physicsDesc.angularDamping;

    {
        const bool is_near_zero = MathUtils::IsEquivalentToZero(new_angular_velocity);
        const bool is_inf = std::isinf(new_angular_velocity);
        const bool is_nan = std::isnan(new_angular_velocity);
        const bool should_clamp = is_near_zero || is_nan || is_inf;
        if(should_clamp) {
            new_angular_velocity = 0.0f;
        }
    }
    const auto new_orientationDegrees = MathUtils::Wrap(new_angular_velocity + new_angular_acceleration * t * t, 0.0f, 360.0f);
    prev_orientationDegrees = orientationDegrees;
    orientationDegrees = new_orientationDegrees;

    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto S = Matrix4::CreateScaleMatrix(collider->GetHalfExtents());
        const auto R = Matrix4::Create2DRotationDegreesMatrix(orientationDegrees);
        const auto T = Matrix4::CreateTranslationMatrix(position);
        const auto M = Matrix4::MakeSRT(S, R, T);
        auto new_transform = Matrix4::I;
        if(!parent) {
            new_transform = M;
        } else {
            auto p = parent;
            while(p) {
                new_transform = Matrix4::MakeRT(p->GetParentTransform(), M);
                p = p->parent;
            }
        }
        transform = new_transform;
        collider->SetPosition(position);
        collider->SetOrientationDegrees(orientationDegrees);
    }

    for(auto& force : linear_forces) {
        force.second -= deltaSeconds;
    }
    for(auto& force : angular_forces) {
        force.second -= deltaSeconds;
    }
}

void RigidBody::DebugRender() const {
    auto& renderer = ServiceLocator::get<IRendererService>();
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto he = this->GetBounds().half_extents * 2.0f;
        const auto S = Matrix4::CreateScaleMatrix(he);
        const auto R = Matrix4::Create2DRotationDegreesMatrix(GetOrientationDegrees());
        const auto T = Matrix4::CreateTranslationMatrix(GetPosition());
        const auto M = Matrix4::MakeSRT(S, R, T);
        renderer.SetModelMatrix(M);
        collider->DebugRender();
        renderer.DrawOBB2(GetOrientationDegrees(), Rgba::Green);
    }
}

void RigidBody::Endframe() {
    if(should_kill) {
        auto& physics = ServiceLocator::get<IPhysicsService>();
        physics.RemoveObject(this);
    }
}

void RigidBody::EnablePhysics(bool enabled) {
    rigidbodyDesc.physicsDesc.enablePhysics = enabled;
}

void RigidBody::EnableGravity(bool enabled) {
    rigidbodyDesc.physicsDesc.enableGravity = IsDynamic() && enabled;
}

void RigidBody::EnableDrag(bool enabled) {
    rigidbodyDesc.physicsDesc.enableDrag = IsDynamic() && enabled;
}

bool RigidBody::IsPhysicsEnabled() const {
    return rigidbodyDesc.physicsDesc.enablePhysics;
}

bool RigidBody::IsGravityEnabled() const {
    return IsDynamic() && rigidbodyDesc.physicsDesc.enableGravity;
}

bool RigidBody::IsDragEnabled() const {
    return IsDynamic() && rigidbodyDesc.physicsDesc.enableDrag;
}

bool RigidBody::IsDynamic() const noexcept {
    return rigidbodyDesc.collider != nullptr;
}

void RigidBody::SetAwake(bool awake) noexcept {
    is_awake = IsDynamic() && awake;
}

void RigidBody::Wake() noexcept {
    SetAwake(true);
}

void RigidBody::Sleep() noexcept {
    SetAwake(false);
}

bool RigidBody::IsAwake() const {
    return IsDynamic() && is_awake;
}

float RigidBody::GetMass() const {
    return rigidbodyDesc.physicsDesc.mass;
}

float RigidBody::GetInverseMass() const {
    if(const auto mass = GetMass(); mass > 0.0f) {
        return 1.0f / mass;
    }
    return 0.0f;
}

Matrix4 RigidBody::GetParentTransform() const {
    if(parent) {
        return parent->transform;
    }
    return Matrix4::I;
}

void RigidBody::ApplyImpulse(const Vector2& impulse) {
    SetAwake(true);
    linear_impulses.push_back(impulse);
}

void RigidBody::ApplyImpulse(const Vector2& direction, float magnitude) {
    ApplyImpulse(direction.GetNormalize() * magnitude);
}

void RigidBody::ApplyForce(const Vector2& force, const TimeUtils::FPSeconds& duration) {
    SetAwake(true);
    linear_forces.push_back(std::make_pair(force, duration));
}

void RigidBody::ApplyForce(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyForce(direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyTorque(float force, const TimeUtils::FPSeconds& duration) {
    SetAwake(true);
    if(!IsRotationLocked()) {
        if(duration == TimeUtils::FPSeconds::zero()) {
            angular_impulses.push_back(force);
        } else {
            angular_forces.push_back(std::make_pair(force, duration));
        }
    }
}

void RigidBody::ApplyTorqueAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyTorqueAt(position_on_object, direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyTorqueAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration) {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto point_of_collision = MathUtils::CalcClosestPoint(position_on_object, *collider);
        const auto r = position - point_of_collision;
        const auto torque = MathUtils::CrossProduct(force, r);
        ApplyTorque(torque, duration);
    }
}

void RigidBody::ApplyTorque(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyTorqueAt(position, direction * magnitude, duration);
}

void RigidBody::ApplyForceAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyForceAt(position_on_object, direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyForceAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration) {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto point_of_collision = MathUtils::CalcClosestPoint(position_on_object, *collider);
        auto r = position - point_of_collision;
        if(MathUtils::IsEquivalentToZero(r)) {
            r = position;
        }
        const auto&& [parallel, perpendicular] = MathUtils::DivideIntoProjectAndReject(force, r);
        const auto angular_result = force - parallel;
        const auto linear_result = force - perpendicular;
        ApplyTorqueAt(position_on_object, angular_result, duration);
        ApplyForce(linear_result, duration);
    }
}

void RigidBody::ApplyImpulseAt(const Vector2& position_on_object, const Vector2& direction, float magnitude) {
    ApplyImpulseAt(position_on_object, direction.GetNormalize() * magnitude);
}

void RigidBody::ApplyImpulseAt(const Vector2& position_on_object, const Vector2& force) {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto point_of_collision = MathUtils::CalcClosestPoint(position_on_object, *collider);
        const auto r = position - point_of_collision;
        const auto&& [parallel, perpendicular] = MathUtils::DivideIntoProjectAndReject(force, r);
        const auto angular_result = force - parallel;
        const auto linear_result = force - perpendicular;
        ApplyTorqueAt(position_on_object, angular_result.GetNormalize(), angular_result.CalcLength(), TimeUtils::FPSeconds::zero());
        ApplyImpulse(linear_result);
    }
}

const OBB2 RigidBody::GetBounds() const {
    const auto& center = GetPosition();
    const auto dims = CalcDimensions();
    const auto orientation = GetOrientationDegrees();
    return OBB2(center, dims * 0.5f, orientation);
}

void RigidBody::SetPosition(const Vector2& newPosition, bool teleport /*= false*/) noexcept {
    if(teleport) {
        position = newPosition;
    } else {
        Wake();
        position = newPosition;
    }
}

const Vector2& RigidBody::GetPosition() const {
    return position;
}

void RigidBody::SetVelocity(const Vector2& newVelocity) noexcept {
    velocity = newVelocity;
}

const Vector2& RigidBody::GetVelocity() const {
    return velocity;
}

const Vector2& RigidBody::GetAcceleration() const {
    return acceleration;
}

Vector2 RigidBody::CalcDimensions() const {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        return collider->CalcDimensions();
    }
    return Vector2::Zero;
}

float RigidBody::GetOrientationDegrees() const {
    return orientationDegrees;
}

float RigidBody::GetAngularVelocityDegrees() const {
    return (orientationDegrees - prev_orientationDegrees) / dt.count();
}

float RigidBody::GetAngularAccelerationDegrees() const {
    return angular_acceleration;
}

const Collider* RigidBody::GetCollider() const noexcept {
    return rigidbodyDesc.collider;
}

Collider* RigidBody::GetCollider() noexcept {
    return rigidbodyDesc.collider;
}

void RigidBody::FellOutOfWorld() noexcept {
    should_kill = true;
}

const bool RigidBody::ShouldKill() const noexcept {
    return should_kill;
}

const bool RigidBody::IsRotationLocked() const noexcept {
    return should_lock_rotation;
}

void RigidBody::LockRotation(bool shouldLockRotation) noexcept {
    shouldLockRotation = shouldLockRotation;
}

void RigidBody::SetAcceleration(const Vector2& newAccleration) noexcept {
    acceleration = newAccleration;
}

Vector2 RigidBody::CalcForceVector() noexcept {
    //const auto inv_mass = GetInverseMass();
    const auto linear_impulse_sum = std::accumulate(std::begin(linear_impulses), std::end(linear_impulses), Vector2::Zero);

    using LinearForceType = typename std::decay<decltype(*linear_forces.begin())>::type;
    const auto linear_acc = [](const LinearForceType& a, const LinearForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto linear_force_sum = std::accumulate(std::begin(linear_forces), std::end(linear_forces), std::make_pair(Vector2::Zero, TimeUtils::FPSeconds::zero()), linear_acc);

    return linear_impulse_sum + linear_force_sum.first;
}
