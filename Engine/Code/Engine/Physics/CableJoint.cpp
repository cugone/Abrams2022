#include "Engine/Physics/CableJoint.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Physics/PhysicsUtils.hpp"
#include "Engine/Physics/RigidBody.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

CableJoint::CableJoint(const CableJointDef& def) noexcept {
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
    m_def.length = def.length;
}

void CableJoint::Notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* first_body = GetBodyA();
    auto* second_body = GetBodyB();
    if(first_body == nullptr && second_body == nullptr) {
        return;
    }

    const auto fb_pos = GetAnchorA();
    const auto sb_pos = GetAnchorB();

    const auto distance = MathUtils::CalcDistance(fb_pos, sb_pos);
    const auto displacement_towards_first = fb_pos - sb_pos;
    const auto displacement_towards_second = sb_pos - fb_pos;
    const auto direction_to_first = displacement_towards_first.GetNormalize();
    const auto direction_to_second = displacement_towards_second.GetNormalize();
    const auto m1 = (first_body ? first_body->GetMass() : 0.0f);
    const auto m2 = (second_body ? second_body->GetMass() : 0.0f);
    const auto mass_sum = m1 + m2;
    const auto mass1_ratio = m1 / mass_sum;
    const auto mass2_ratio = m2 / mass_sum;
    const auto length = m_def.length;
    if(length < distance) {
        if(first_body) {
            first_body->ApplyImpulse(direction_to_second * mass1_ratio);
        }
        if(second_body) {
            second_body->ApplyImpulse(direction_to_first * mass2_ratio);
        }
    }
}

void CableJoint::DebugRender() const noexcept {
    if(!(m_def.rigidBodyA || m_def.rigidBodyB)) {
        return;
    }
    const auto posA = GetAnchorA();
    const auto posB = GetAnchorB();
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetModelMatrix(Matrix4::I);
    renderer.DrawLine2D(posA, posB);
}

void CableJoint::Attach(RigidBody* a, RigidBody* b, Vector2 localAnchorA /*= Vector2::ZERO*/, Vector2 localAnchorB /*= Vector2::ZERO*/) noexcept {
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

void CableJoint::Detach(const RigidBody* body) noexcept {
    if(body == m_def.rigidBodyA) {
        m_def.rigidBodyA = nullptr;
    } else if(body == m_def.rigidBodyB) {
        m_def.rigidBodyB = nullptr;
    }
}

void CableJoint::DetachAll() noexcept {
    m_def.rigidBodyA = nullptr;
    m_def.rigidBodyB = nullptr;
}

bool CableJoint::IsNotAttached() const noexcept {
    return m_def.rigidBodyA == nullptr || m_def.rigidBodyB == nullptr;
}

RigidBody* CableJoint::GetBodyA() const noexcept {
    return m_def.rigidBodyA;
}

RigidBody* CableJoint::GetBodyB() const noexcept {
    return m_def.rigidBodyB;
}

Vector2 CableJoint::GetAnchorA() const noexcept {
    return m_def.rigidBodyA ? m_def.rigidBodyA->GetPosition() + (m_def.rigidBodyA->CalcDimensions() * 0.5f * m_def.localAnchorA) : m_def.worldAnchorA;
}

Vector2 CableJoint::GetAnchorB() const noexcept {
    return m_def.rigidBodyB ? m_def.rigidBodyB->GetPosition() + (m_def.rigidBodyB->CalcDimensions() * 0.5f * m_def.localAnchorB) : m_def.worldAnchorB;
}

float CableJoint::GetMassA() const noexcept {
    return m_def.rigidBodyA ? m_def.rigidBodyA->GetMass() : 0.0f;
}

float CableJoint::GetMassB() const noexcept {
    return m_def.rigidBodyB ? m_def.rigidBodyB->GetMass() : 0.0f;
}

bool CableJoint::ConstraintViolated() const noexcept {
    const bool violated = [this]() -> const bool {
        const auto distance = MathUtils::CalcDistance(GetAnchorA(), GetAnchorB());
        return m_def.length < distance;
    }();
    return violated;
}

void CableJoint::SolvePositionConstraint() const noexcept {
    auto* first_body = GetBodyA();
    auto* second_body = GetBodyB();
    if(first_body == nullptr && second_body == nullptr) {
        return;
    }
    const auto posA = GetAnchorA();
    const auto posB = GetAnchorB();

    const auto distance = MathUtils::CalcDistance(posA, posB);
    const auto displacement_towards_first = posA - posB;
    const auto displacement_towards_second = posB - posA;
    const auto direction_to_first = displacement_towards_first.GetNormalize();
    const auto direction_to_second = displacement_towards_second.GetNormalize();
    const auto m1 = (first_body ? first_body->GetMass() : 0.0f);
    const auto m2 = (second_body ? second_body->GetMass() : 0.0f);
    const auto mass_sum = m1 + m2;
    const auto mass1_ratio = m1 / mass_sum;
    const auto mass2_ratio = m2 / mass_sum;
    const auto length = m_def.length;
    auto newPosition1 = posA;
    auto newPosition2 = posB;
    if(length < distance) {
        if(first_body) {
            const auto newDisplacement = mass1_ratio * direction_to_second * std::abs(length - distance);
            const auto newPosition = first_body->GetPosition() + newDisplacement;
            newPosition1 = newPosition;
        }
        if(second_body) {
            const auto newDisplacement = mass2_ratio * direction_to_first * std::abs(length - distance);
            const auto newPosition = second_body->GetPosition() + newDisplacement;
            newPosition2 = newPosition;
        }
    }
    if(first_body) {
        first_body->SetPosition(newPosition1, true);
    }
    if(second_body) {
        second_body->SetPosition(newPosition2, true);
    }
}

void CableJoint::SolveVelocityConstraint() const noexcept {
    auto* first_body = GetBodyA();
    auto* second_body = GetBodyB();
    if(first_body == nullptr && second_body == nullptr) {
        return;
    }

    const auto posA = GetAnchorA();
    const auto posB = GetAnchorB();

    const auto displacement_towards_first = posA - posB;
    const auto displacement_towards_second = posB - posA;
    const auto direction_to_first = displacement_towards_first.GetNormalize();
    const auto direction_to_second = displacement_towards_second.GetNormalize();
    const auto m1 = GetMassA();
    const auto m2 = GetMassB();
    const auto mass_sum = m1 + m2;
    const auto mass1_ratio = m1 / mass_sum;
    const auto mass2_ratio = m2 / mass_sum;
    auto v1 = first_body ? first_body->GetVelocity() : Vector2::Zero;
    auto v2 = second_body ? second_body->GetVelocity() : Vector2::Zero;
    auto newVelocity1 = v1;
    auto newVelocity2 = v2;
    newVelocity1 = mass1_ratio * MathUtils::Reject(v1, direction_to_second);
    newVelocity2 = mass2_ratio * MathUtils::Reject(v2, direction_to_first);
    if(first_body) {
        first_body->SetVelocity(newVelocity1);
    }
    if(second_body) {
        second_body->SetVelocity(newVelocity2);
    }
}
