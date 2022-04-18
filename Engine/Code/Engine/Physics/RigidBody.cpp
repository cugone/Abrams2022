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
: m_rigidbodyDesc(desc)
, m_velocity(m_rigidbodyDesc.initialVelocity)
, m_position(m_rigidbodyDesc.initialPosition)
, m_acceleration(m_rigidbodyDesc.initialAcceleration) {
    const auto area = m_rigidbodyDesc.collider->CalcArea();
    if(MathUtils::IsEquivalentToZero(m_rigidbodyDesc.physicsMaterial.density) || MathUtils::IsEquivalentToZero(area)) {
        m_rigidbodyDesc.physicsDesc.mass = 0.0f;
    } else {
        m_rigidbodyDesc.physicsDesc.mass = m_rigidbodyDesc.physicsMaterial.density * area;
        m_rigidbodyDesc.physicsDesc.mass = std::pow(m_rigidbodyDesc.physicsDesc.mass, m_rigidbodyDesc.physicsMaterial.massExponent);
        if(!MathUtils::IsEquivalentToZero(m_rigidbodyDesc.physicsDesc.mass) && m_rigidbodyDesc.physicsDesc.mass < 0.001f) {
            m_rigidbodyDesc.physicsDesc.mass = 0.001f;
        }
    }
}

void RigidBody::BeginFrame() {
    static constexpr auto pred = [](const auto& force) { return force.second.count() <= 0.0f; };
    if(!m_linear_forces.empty()) {
        m_linear_forces.erase(std::remove_if(m_linear_forces.begin(), m_linear_forces.end(), pred), m_linear_forces.end());
    }
    if(!m_angular_forces.empty()) {
        m_angular_forces.erase(std::remove_if(m_angular_forces.begin(), m_angular_forces.end(), pred), m_angular_forces.end());
    }
}

void RigidBody::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(!(IsPhysicsEnabled() && IsDynamic() && IsAwake()) || MathUtils::IsEquivalentToZero(GetInverseMass())) {
        m_linear_impulses.clear();
        m_angular_impulses.clear();
        m_linear_forces.clear();
        m_angular_forces.clear();
        Integrate(deltaSeconds);
        return;
    }

    Integrate(deltaSeconds);

    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto S = Matrix4::CreateScaleMatrix(collider->GetHalfExtents());
        const auto R = Matrix4::Create2DRotationDegreesMatrix(m_orientationDegrees);
        const auto T = Matrix4::CreateTranslationMatrix(m_position);
        const auto M = Matrix4::MakeSRT(S, R, T);
        auto new_transform = Matrix4::I;
        if(!m_parent) {
            new_transform = M;
        } else {
            auto p = m_parent;
            while(p) {
                new_transform = Matrix4::MakeRT(p->GetParentTransform(), M);
                p = p->m_parent;
            }
        }
        transform = new_transform;
        collider->SetPosition(m_position);
        collider->SetOrientationDegrees(m_orientationDegrees);
    }

    for(auto& force : m_linear_forces) {
        force.second -= deltaSeconds;
    }
    for(auto& force : m_angular_forces) {
        force.second -= deltaSeconds;
    }
}

void RigidBody::Integrate(TimeUtils::FPSeconds deltaSeconds) noexcept {
    const auto inv_mass = GetInverseMass();
    const auto linear_impulse_sum = std::accumulate(std::begin(m_linear_impulses), std::end(m_linear_impulses), Vector2::Zero);
    const auto angular_impulse_sum = std::accumulate(std::begin(m_angular_impulses), std::end(m_angular_impulses), 0.0f);
    m_linear_impulses.clear();
    m_angular_impulses.clear();

    using LinearForceType = typename std::decay<decltype(*m_linear_forces.begin())>::type;
    const auto linear_acc = [](const LinearForceType& a, const LinearForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto linear_force_sum = std::accumulate(std::begin(m_linear_forces), std::end(m_linear_forces), std::make_pair(Vector2::Zero, TimeUtils::FPSeconds::zero()), linear_acc);

    using AngularForceType = typename std::decay<decltype(*m_angular_forces.begin())>::type;
    const auto angular_acc = [](const AngularForceType& a, const AngularForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto angular_force_sum = std::accumulate(std::begin(m_angular_forces), std::end(m_angular_forces), std::make_pair(0.0f, TimeUtils::FPSeconds::zero()), angular_acc);

    //https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet

    const auto new_angular_acceleration = (angular_impulse_sum + angular_force_sum.first) * inv_mass;
    const auto t = deltaSeconds.count();
    m_dt = deltaSeconds;

    auto new_position = GetPosition() + GetVelocity() * m_dt.count() + GetAcceleration() * (m_dt.count() * m_dt.count() * 0.5f);
    auto new_acceleration = (linear_impulse_sum + linear_force_sum.first) * inv_mass;
    auto new_velocity = GetVelocity() + (GetAcceleration() + new_acceleration) * (m_dt.count() * 0.5f);
    new_velocity *= std::clamp(1.0f - m_rigidbodyDesc.physicsDesc.linearDamping, 0.0f, 1.0f);

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

    auto deltaPosition = m_position - new_position;
    auto deltaOrientation = m_orientationDegrees - m_prev_orientationDegrees;
    if(MathUtils::IsEquivalentToZero(deltaPosition)
       && MathUtils::IsEquivalentToZero(deltaOrientation)) {
        m_time_since_last_move += m_dt;
    } else {
        m_time_since_last_move = TimeUtils::FPSeconds{0.0f};
    }

    SetPosition(new_position, true);
    SetVelocity(new_velocity);
    SetAcceleration(new_acceleration);

    const auto& maxAngularSpeed = m_rigidbodyDesc.physicsDesc.maxAngularSpeed;
    auto new_angular_velocity = std::clamp((2.0f * m_orientationDegrees - m_prev_orientationDegrees) / m_dt.count(), -maxAngularSpeed, maxAngularSpeed);
    new_angular_velocity *= std::clamp(1.0f - m_rigidbodyDesc.physicsDesc.angularDamping, 0.0f, 1.0f);

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
    m_prev_orientationDegrees = m_orientationDegrees;
    m_orientationDegrees = new_orientationDegrees;
}

void RigidBody::DebugRender() const {
    auto& renderer = ServiceLocator::get<IRendererService>();
    if(auto* const collider = GetCollider(); collider != nullptr) {
        renderer.SetModelMatrix(transform);
        collider->DebugRender();
        renderer.DrawOBB2(GetOrientationDegrees(), Rgba::Green);
    }
}

void RigidBody::Endframe() {
    if(m_should_kill) {
        auto& physics = ServiceLocator::get<IPhysicsService>();
        physics.RemoveObject(this);
    }
}

void RigidBody::EnablePhysics(bool enabled) {
    m_rigidbodyDesc.physicsDesc.enablePhysics = enabled;
}

void RigidBody::EnableGravity(bool enabled) {
    m_rigidbodyDesc.physicsDesc.enableGravity = IsDynamic() && enabled;
}

void RigidBody::EnableDrag(bool enabled) {
    m_rigidbodyDesc.physicsDesc.enableDrag = IsDynamic() && enabled;
}

bool RigidBody::IsPhysicsEnabled() const {
    return m_rigidbodyDesc.physicsDesc.enablePhysics;
}

bool RigidBody::IsGravityEnabled() const {
    return IsDynamic() && m_rigidbodyDesc.physicsDesc.enableGravity;
}

bool RigidBody::IsDragEnabled() const {
    return IsDynamic() && m_rigidbodyDesc.physicsDesc.enableDrag;
}

bool RigidBody::IsDynamic() const noexcept {
    return m_rigidbodyDesc.collider != nullptr;
}

void RigidBody::SetAwake(bool awake) noexcept {
    m_is_awake = IsDynamic() && awake;
}

void RigidBody::Wake() noexcept {
    SetAwake(true);
}

void RigidBody::Sleep() noexcept {
    SetAwake(false);
}

bool RigidBody::IsAwake() const {
    return IsDynamic() && m_is_awake;
}

float RigidBody::GetMass() const {
    return m_rigidbodyDesc.physicsDesc.mass;
}

float RigidBody::GetInverseMass() const {
    if(const auto mass = GetMass(); mass > 0.0f) {
        return 1.0f / mass;
    }
    return 0.0f;
}

Matrix4 RigidBody::GetParentTransform() const {
    if(m_parent) {
        return m_parent->transform;
    }
    return Matrix4::I;
}

void RigidBody::ApplyImpulse(const Vector2& impulse) {
    m_linear_impulses.push_back(impulse);
}

void RigidBody::ApplyImpulse(const Vector2& direction, float magnitude) {
    ApplyImpulse(direction.GetNormalize() * magnitude);
}

void RigidBody::ApplyForce(const Vector2& force, const TimeUtils::FPSeconds& duration) {
    m_linear_forces.push_back(std::make_pair(force, duration));
}

void RigidBody::ApplyForce(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyForce(direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyTorque(float force, const TimeUtils::FPSeconds& duration) {
    if(!IsRotationLocked()) {
        if(duration == TimeUtils::FPSeconds::zero()) {
            m_angular_impulses.push_back(force);
        } else {
            m_angular_forces.push_back(std::make_pair(force, duration));
        }
    }
}

void RigidBody::ApplyTorqueAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyTorqueAt(position_on_object, direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyTorqueAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration) {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto point_of_collision = MathUtils::CalcClosestPoint(position_on_object, *collider);
        const auto r = m_position - point_of_collision;
        const auto torque = MathUtils::CrossProduct(force, r);
        ApplyTorque(torque, duration);
    }
}

void RigidBody::ApplyTorque(const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyTorqueAt(m_position, direction * magnitude, duration);
}

void RigidBody::ApplyForceAt(const Vector2& position_on_object, const Vector2& direction, float magnitude, const TimeUtils::FPSeconds& duration) {
    ApplyForceAt(position_on_object, direction.GetNormalize() * magnitude, duration);
}

void RigidBody::ApplyForceAt(const Vector2& position_on_object, const Vector2& force, const TimeUtils::FPSeconds& duration) {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        const auto point_of_collision = MathUtils::CalcClosestPoint(position_on_object, *collider);
        auto r = m_position - point_of_collision;
        if(MathUtils::IsEquivalentToZero(r)) {
            r = m_position;
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
        const auto r = m_position - point_of_collision;
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
        m_position = newPosition;
    } else {
        Wake();
        m_position = newPosition;
    }
}

const Vector2& RigidBody::GetPosition() const {
    return m_position;
}

void RigidBody::SetVelocity(const Vector2& newVelocity) noexcept {
    m_velocity = newVelocity;
}

const Vector2& RigidBody::GetVelocity() const {
    return m_velocity;
}

const Vector2& RigidBody::GetAcceleration() const {
    return m_acceleration;
}

Vector2 RigidBody::CalcDimensions() const {
    if(auto* const collider = GetCollider(); collider != nullptr) {
        return collider->CalcDimensions();
    }
    return Vector2::Zero;
}

float RigidBody::GetOrientationDegrees() const {
    return m_orientationDegrees;
}

float RigidBody::GetAngularVelocityDegrees() const {
    return (m_orientationDegrees - m_prev_orientationDegrees) / m_dt.count();
}

float RigidBody::GetAngularAccelerationDegrees() const {
    return m_angular_acceleration;
}

const Collider* RigidBody::GetCollider() const noexcept {
    return m_rigidbodyDesc.collider;
}

Collider* RigidBody::GetCollider() noexcept {
    return m_rigidbodyDesc.collider;
}

void RigidBody::FellOutOfWorld() noexcept {
    m_should_kill = true;
}

const bool RigidBody::ShouldKill() const noexcept {
    return m_should_kill;
}

const bool RigidBody::IsRotationLocked() const noexcept {
    return m_should_lock_rotation;
}

void RigidBody::LockRotation(bool shouldLockRotation) noexcept {
    shouldLockRotation = shouldLockRotation;
}

void RigidBody::SetAcceleration(const Vector2& newAccleration) noexcept {
    m_acceleration = newAccleration;
}

Vector2 RigidBody::CalcForceVector() noexcept {
    //const auto inv_mass = GetInverseMass();
    const auto linear_impulse_sum = std::accumulate(std::begin(m_linear_impulses), std::end(m_linear_impulses), Vector2::Zero);

    using LinearForceType = typename std::decay<decltype(*m_linear_forces.begin())>::type;
    const auto linear_acc = [](const LinearForceType& a, const LinearForceType& b) { return std::make_pair(a.first + b.first, TimeUtils::FPSeconds::zero()); };
    const auto linear_force_sum = std::accumulate(std::begin(m_linear_forces), std::end(m_linear_forces), std::make_pair(Vector2::Zero, TimeUtils::FPSeconds::zero()), linear_acc);

    return linear_impulse_sum + linear_force_sum.first;
}
