#include "Engine/Physics/SpringJoint.hpp"

#include "Engine/Physics/PhysicsUtils.hpp"
#include "Engine/Physics/RigidBody.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

SpringJoint::SpringJoint(const SpringJointDef& def) noexcept {
    m_def.rigidBodyA = def.rigidBodyA;
    m_def.rigidBodyB = def.rigidBodyB;
    m_def.localAnchorA = def.localAnchorA;
    m_def.localAnchorB = def.localAnchorB;
    m_def.linearDamping = def.linearDamping;
    m_def.angularDamping = def.angularDamping;
    m_def.attachedCollidable = def.attachedCollidable;
    m_def.breakForce = def.breakForce;
    m_def.breakTorque = def.breakTorque;
    auto posA = m_def.localAnchorA;
    auto posB = m_def.localAnchorB;
    if(m_def.rigidBodyA) {
        posA = m_def.rigidBodyA->GetPosition() + (m_def.rigidBodyA->CalcDimensions() * 0.5f * m_def.localAnchorA);
    }
    if(m_def.rigidBodyB) {
        posB = m_def.rigidBodyB->GetPosition() + (m_def.rigidBodyB->CalcDimensions() * 0.5f * m_def.localAnchorB);
    }
    m_def.worldAnchorA = posA;
    m_def.worldAnchorB = posB;
    m_def.k = def.k;
    m_def.length = def.length;
}

void SpringJoint::Notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* first_body = GetBodyA();
    auto* second_body = GetBodyB();
    if(first_body == nullptr || second_body == nullptr) {
        return;
    }

    auto left_direction = Vector2{first_body->GetPosition() - second_body->GetPosition()};
    auto left_magnitude = left_direction.Normalize();
    auto current_compression = left_magnitude - m_def.length;
    left_magnitude = m_def.k * current_compression;

    auto right_direction = Vector2{second_body->GetPosition() - first_body->GetPosition()};
    auto right_magnitude = right_direction.Normalize();
    current_compression = right_magnitude - m_def.length;
    right_magnitude = m_def.k * current_compression;

    //Apply Right Force to Left Object.
    first_body->ApplyImpulse(right_direction * right_magnitude);
    //Apply Left Force to Right Object.
    second_body->ApplyImpulse(left_direction * left_magnitude);
}

void SpringJoint::DebugRender() const noexcept {
    if(!(m_def.rigidBodyA || m_def.rigidBodyB)) {
        return;
    }
    const auto posA = GetAnchorA();
    const auto posB = GetAnchorB();

    auto* renderer = ServiceLocator::get<IRendererService, NullRendererService>();
    renderer->SetModelMatrix(Matrix4::I);
    renderer->DrawLine2D(posA, posB);
}

void SpringJoint::Attach(RigidBody* a, RigidBody* b, Vector2 localAnchorA /*= Vector2::ZERO*/, Vector2 localAnchorB /*= Vector2::ZERO*/) noexcept {
    m_def.rigidBodyA = a;
    m_def.rigidBodyB = b;
    m_def.localAnchorA = localAnchorA;
    m_def.localAnchorB = localAnchorB;
    if(a) {
        m_def.worldAnchorA = m_def.rigidBodyA->GetPosition() + (m_def.rigidBodyA->CalcDimensions() * 0.5f * m_def.localAnchorA);
    }
    if(b) {
        m_def.worldAnchorB = m_def.rigidBodyB->GetPosition() + (m_def.rigidBodyB->CalcDimensions() * 0.5f * m_def.localAnchorB);
    }
}

void SpringJoint::Detach(const RigidBody* body) noexcept {
    if(body == m_def.rigidBodyA) {
        m_def.rigidBodyA = nullptr;
    } else if(body == m_def.rigidBodyB) {
        m_def.rigidBodyB = nullptr;
    }
}

void SpringJoint::DetachAll() noexcept {
    m_def.rigidBodyA = nullptr;
    m_def.rigidBodyB = nullptr;
}

bool SpringJoint::IsNotAttached() const noexcept {
    return m_def.rigidBodyA == nullptr || m_def.rigidBodyB == nullptr;
}

RigidBody* SpringJoint::GetBodyA() const noexcept {
    return m_def.rigidBodyA;
}

RigidBody* SpringJoint::GetBodyB() const noexcept {
    return m_def.rigidBodyB;
}

Vector2 SpringJoint::GetAnchorA() const noexcept {
    return m_def.rigidBodyA ? m_def.rigidBodyA->GetPosition() + (m_def.rigidBodyA->CalcDimensions() * 0.5f * m_def.localAnchorA) : m_def.worldAnchorA;
}

Vector2 SpringJoint::GetAnchorB() const noexcept {
    return m_def.rigidBodyB ? m_def.rigidBodyB->GetPosition() + (m_def.rigidBodyB->CalcDimensions() * 0.5f * m_def.localAnchorB) : m_def.worldAnchorB;
}

float SpringJoint::GetMassA() const noexcept {
    return m_def.rigidBodyA ? m_def.rigidBodyA->GetMass() : 0.0f;
}

float SpringJoint::GetMassB() const noexcept {
    return m_def.rigidBodyB ? m_def.rigidBodyB->GetMass() : 0.0f;
}

bool SpringJoint::ConstraintViolated() const noexcept {
    return false;
}

void SpringJoint::SolvePositionConstraint() const noexcept {
    /* DO NOTHING */
}

void SpringJoint::SolveVelocityConstraint() const noexcept {
    /* DO NOTHING */
}
