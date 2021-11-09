#include "Engine/Physics/SpringJoint.hpp"

#include "Engine/Physics/PhysicsUtils.hpp"
#include "Engine/Physics/RigidBody.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

SpringJoint::SpringJoint(const SpringJointDef& def) noexcept {
    _def.rigidBodyA = def.rigidBodyA;
    _def.rigidBodyB = def.rigidBodyB;
    _def.localAnchorA = def.localAnchorA;
    _def.localAnchorB = def.localAnchorB;
    _def.linearDamping = def.linearDamping;
    _def.angularDamping = def.angularDamping;
    _def.attachedCollidable = def.attachedCollidable;
    _def.breakForce = def.breakForce;
    _def.breakTorque = def.breakTorque;
    auto posA = _def.localAnchorA;
    auto posB = _def.localAnchorB;
    if(_def.rigidBodyA) {
        posA = _def.rigidBodyA->GetPosition() + (_def.rigidBodyA->CalcDimensions() * 0.5f * _def.localAnchorA);
    }
    if(_def.rigidBodyB) {
        posB = _def.rigidBodyB->GetPosition() + (_def.rigidBodyB->CalcDimensions() * 0.5f * _def.localAnchorB);
    }
    _def.worldAnchorA = posA;
    _def.worldAnchorB = posB;
    _def.k = def.k;
    _def.length = def.length;
}

void SpringJoint::Notify([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    auto* first_body = GetBodyA();
    auto* second_body = GetBodyB();
    if(first_body == nullptr || second_body == nullptr) {
        return;
    }

    auto left_direction = Vector2{first_body->GetPosition() - second_body->GetPosition()};
    auto left_magnitude = left_direction.Normalize();
    auto current_compression = left_magnitude - _def.length;
    left_magnitude = _def.k * current_compression;

    auto right_direction = Vector2{second_body->GetPosition() - first_body->GetPosition()};
    auto right_magnitude = right_direction.Normalize();
    current_compression = right_magnitude - _def.length;
    right_magnitude = _def.k * current_compression;

    //Apply Right Force to Left Object.
    first_body->ApplyImpulse(right_direction * right_magnitude);
    //Apply Left Force to Right Object.
    second_body->ApplyImpulse(left_direction * left_magnitude);
}

void SpringJoint::DebugRender() const noexcept {
    if(!(_def.rigidBodyA || _def.rigidBodyB)) {
        return;
    }
    const auto posA = GetAnchorA();
    const auto posB = GetAnchorB();

    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetModelMatrix(Matrix4::I);
    renderer.DrawLine2D(posA, posB);
}

void SpringJoint::Attach(RigidBody* a, RigidBody* b, Vector2 localAnchorA /*= Vector2::ZERO*/, Vector2 localAnchorB /*= Vector2::ZERO*/) noexcept {
    _def.rigidBodyA = a;
    _def.rigidBodyB = b;
    _def.localAnchorA = localAnchorA;
    _def.localAnchorB = localAnchorB;
    if(a) {
        _def.worldAnchorA = _def.rigidBodyA->GetPosition() + (_def.rigidBodyA->CalcDimensions() * 0.5f * _def.localAnchorA);
    }
    if(b) {
        _def.worldAnchorB = _def.rigidBodyB->GetPosition() + (_def.rigidBodyB->CalcDimensions() * 0.5f * _def.localAnchorB);
    }
}

void SpringJoint::Detach(const RigidBody* body) noexcept {
    if(body == _def.rigidBodyA) {
        _def.rigidBodyA = nullptr;
    } else if(body == _def.rigidBodyB) {
        _def.rigidBodyB = nullptr;
    }
}

void SpringJoint::DetachAll() noexcept {
    _def.rigidBodyA = nullptr;
    _def.rigidBodyB = nullptr;
}

bool SpringJoint::IsNotAttached() const noexcept {
    return _def.rigidBodyA == nullptr || _def.rigidBodyB == nullptr;
}

RigidBody* SpringJoint::GetBodyA() const noexcept {
    return _def.rigidBodyA;
}

RigidBody* SpringJoint::GetBodyB() const noexcept {
    return _def.rigidBodyB;
}

Vector2 SpringJoint::GetAnchorA() const noexcept {
    return _def.rigidBodyA ? _def.rigidBodyA->GetPosition() + (_def.rigidBodyA->CalcDimensions() * 0.5f * _def.localAnchorA) : _def.worldAnchorA;
}

Vector2 SpringJoint::GetAnchorB() const noexcept {
    return _def.rigidBodyB ? _def.rigidBodyB->GetPosition() + (_def.rigidBodyB->CalcDimensions() * 0.5f * _def.localAnchorB) : _def.worldAnchorB;
}

float SpringJoint::GetMassA() const noexcept {
    return _def.rigidBodyA ? _def.rigidBodyA->GetMass() : 0.0f;
}

float SpringJoint::GetMassB() const noexcept {
    return _def.rigidBodyB ? _def.rigidBodyB->GetMass() : 0.0f;
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
