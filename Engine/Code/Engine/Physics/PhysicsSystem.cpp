#include "Engine/Physics/PhysicsSystem.hpp"

#include "Engine/Math/Plane2.hpp"
#include "Engine/Physics/PhysicsUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <algorithm>
#include <mutex>

void PhysicsSystem::Enable(bool enable) {
    m_is_running = enable;
}

void PhysicsSystem::SetGravity(const Vector2& new_gravity) {
    m_desc.gravity = new_gravity;
    m_gravityFG.SetGravity(m_desc.gravity);
}

Vector2 PhysicsSystem::GetGravity() const noexcept {
    return m_desc.gravity;
}

void PhysicsSystem::SetDragCoefficients(const Vector2& k1k2) {
    m_desc.dragK1K2 = k1k2;
    m_dragFG.SetCoefficients(k1k2);
}

void PhysicsSystem::SetDragCoefficients(float linearCoefficient, float squareCoefficient) {
    SetDragCoefficients(Vector2{linearCoefficient, squareCoefficient});
}

std::pair<float, float> PhysicsSystem::GetDragCoefficients() const noexcept {
    return std::make_pair(m_desc.dragK1K2.x, m_desc.dragK1K2.y);
}

const PhysicsSystemDesc& PhysicsSystem::GetWorldDescription() const noexcept {
    return m_desc;
}

void PhysicsSystem::SetWorldDescription(const PhysicsSystemDesc& new_desc) {
    m_desc = new_desc;
    m_gravityFG.SetGravity(m_desc.gravity);
    m_dragFG.SetCoefficients(m_desc.dragK1K2);
    //_world_partition.SetWorldBounds(_desc.world_bounds);
}

void PhysicsSystem::EnablePhysics(bool isPhysicsEnabled) noexcept {
    for(auto* b : m_rigidBodies) {
        b->EnablePhysics(isPhysicsEnabled);
    }
}

const std::vector<std::unique_ptr<Joint>>& PhysicsSystem::Debug_GetJoints() const noexcept {
    return _joints;
}

const std::vector<RigidBody*>& PhysicsSystem::Debug_GetBodies() const noexcept {
    return m_rigidBodies;
}

void PhysicsSystem::EnableGravity(bool isGravityEnabled) noexcept {
    for(auto* b : m_rigidBodies) {
        b->EnableGravity(isGravityEnabled);
    }
}

void PhysicsSystem::EnableDrag(bool isGravityEnabled) noexcept {
    for(auto* b : m_rigidBodies) {
        b->EnableDrag(isGravityEnabled);
    }
}

PhysicsSystem::PhysicsSystem(const PhysicsSystemDesc& desc /*= PhysicsSystemDesc{}*/)
: m_desc(desc)
//, _world_partition(_desc.world_bounds)
{
    /* DO NOTHING */
}

PhysicsSystem::~PhysicsSystem() {
    m_is_running = false;
}

void PhysicsSystem::Initialize() noexcept {
    //_is_running = true;
    //_update_thread = std::thread(&PhysicsSystem::Update_Worker, this);
    //ThreadUtils::SetThreadDescription(_update_thread, "Physics Async Update");
}

void PhysicsSystem::BeginFrame() noexcept {
    if(!m_is_running) {
        return;
    }

    //_rigidBodies.reserve(_rigidBodies.size() + _pending_addition.size());
    for(auto* a : m_pending_addition) {
        m_rigidBodies.emplace_back(a);
    }
    m_pending_addition.clear();
    m_pending_addition.shrink_to_fit();
    //_world_partition.Clear();
    //_world_partition.Add(_rigidBodies);

    for(auto* body : m_rigidBodies) {
        const auto is_gravity_enabled = body->IsGravityEnabled();
        const auto is_drag_enabled = body->IsDragEnabled();
        is_gravity_enabled ? m_gravityFG.attach(body) : m_gravityFG.detach(body);
        is_drag_enabled ? m_dragFG.attach(body) : m_dragFG.detach(body);
        body->BeginFrame();
    }
}

void PhysicsSystem::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(!this->m_is_running) {
        return;
    }
    m_deltaSeconds = deltaSeconds;
    m_accumulatedTime += m_deltaSeconds;
    if(m_accumulatedTime >= m_targetFrameRate) {
        m_accumulatedTime -= m_targetFrameRate;
        return;
    }
    ApplyGravityAndDrag(m_targetFrameRate);
    ApplyCustomAndJointForces(m_targetFrameRate);
    auto& renderer = ServiceLocator::get<IRendererService>();
    const auto camera_position = Vector2(renderer.GetCamera().GetPosition());
    const auto half_extents = Vector2(renderer.GetOutput()->GetDimensions()) * 0.5f;
    const auto query_area = AABB2(camera_position - half_extents, camera_position + half_extents);
    const auto potential_collisions = BroadPhaseCollision(query_area);
    const auto actual_collisions = NarrowPhaseCollision(potential_collisions, PhysicsUtils::GJK, PhysicsUtils::EPA);
    SolveCollision(actual_collisions);
    SolveConstraints();
    UpdateBodiesInBounds(m_targetFrameRate);
    SolveConstraints();
}

void PhysicsSystem::UpdateBodiesInBounds(TimeUtils::FPSeconds deltaSeconds) noexcept {
    for(auto* body : m_rigidBodies) {
        if(!body) {
            continue;
        }
        body->Update(deltaSeconds);
        //if(!MathUtils::DoOBBsOverlap(OBB2(_desc.world_bounds), body->GetBounds())) {
        //    body->FellOutOfWorld();
        //}
        //if(MathUtils::IsPointInFrontOfPlane(body->GetPosition(), Plane2(Vector2::Y_AXIS, _desc.kill_plane_distance))) {
        //    body->FellOutOfWorld();
        //}
    }
}

void PhysicsSystem::ApplyCustomAndJointForces(TimeUtils::FPSeconds deltaSeconds) noexcept {
    for(auto&& fg : _forceGenerators) {
        fg->notify(deltaSeconds);
    }
    for(auto&& joint : _joints) {
        joint->Notify(deltaSeconds);
    }
}

void PhysicsSystem::ApplyGravityAndDrag(TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_gravityFG.notify(deltaSeconds);
    m_dragFG.notify(deltaSeconds);
}

std::vector<RigidBody*> PhysicsSystem::BroadPhaseCollision(const AABB2& /*query_area*/) noexcept {
    std::vector<RigidBody*> potential_collisions{};
    for(auto iterA = std::begin(m_rigidBodies); iterA != std::end(m_rigidBodies); ++iterA) {
        for(auto iterB = iterA + 1; iterB != std::end(m_rigidBodies); ++iterB) {
            auto* bodyA = *iterA;
            auto* bodyB = *iterB;
            if(!bodyA || !bodyB) {
                continue;
            }
            const auto bodyABoundsAsAABB2 = AABB2{bodyA->GetBounds()};
            const auto bodyBBoundsAsAABB2 = AABB2{bodyB->GetBounds()};
            //if(!MathUtils::DoAABBsOverlap(query_area, bodyBoundsAsAABB2)) {
            //    continue;
            //}
            if(MathUtils::DoAABBsOverlap(bodyABoundsAsAABB2, bodyBBoundsAsAABB2)) {
                //const auto queried_bodies = _world_partition.Query(query_area);
                //for(auto* query : queried_bodies) {
                //potential_collisions.push_back(query);
                //}
                potential_collisions.push_back(bodyA);
                potential_collisions.push_back(bodyB);
            }
        }
    }
    return potential_collisions;
}

void PhysicsSystem::SolveCollision(const PhysicsSystem::CollisionDataSet& actual_collisions) noexcept {
    for(auto& collision : actual_collisions) {
        auto* a = collision.a;
        auto* b = collision.b;
        const auto& aPos = a->GetPosition();
        const auto& bPos = b->GetPosition();
        const auto& aMass = a->GetMass();
        const auto& bMass = b->GetMass();
        const auto massSum = aMass + bMass;
        const auto aNormal = Vector2{-collision.normal};
        const auto bNormal = Vector2{collision.normal};
        const auto aDeltaContribution = (aMass / massSum);
        const auto bDeltaContribution = (bMass / massSum);
        const auto aForceContribution = aNormal * aDeltaContribution;
        const auto bForceContribution = bNormal * bDeltaContribution;
        const auto aDistanceContribution = collision.distance * aDeltaContribution;
        const auto bDistanceContribution = collision.distance * bDeltaContribution;
        a->SetPosition(aPos + aDistanceContribution * aNormal);
        b->SetPosition(bPos + bDistanceContribution * bNormal);
        //collision.a->ApplyForceAt(aPos, aForceContribution, TimeUtils::FPSeconds::zero());
        //collision.b->ApplyForceAt(bPos, bForceContribution, TimeUtils::FPSeconds::zero());
        collision.a->ApplyImpulseAt(aPos, aForceContribution);
        collision.b->ApplyImpulseAt(bPos, bForceContribution);
    }
}

void PhysicsSystem::SolveConstraints() const noexcept {
    for(int i = 0; i < m_desc.position_solver_iterations; ++i) {
        SolvePositionConstraints();
    }
    for(int i = 0; i < m_desc.velocity_solver_iterations; ++i) {
        SolveVelocityConstraints();
    }
}

void PhysicsSystem::SolvePositionConstraints() const noexcept {
    for(auto&& joint : _joints) {
        if(joint->ConstraintViolated()) {
            joint->SolvePositionConstraint();
        }
    }
}

void PhysicsSystem::SolveVelocityConstraints() const noexcept {
    for(auto&& joint : _joints) {
        if(joint->ConstraintViolated()) {
            joint->SolveVelocityConstraint();
        }
    }
}

void PhysicsSystem::Render() const noexcept {
    auto& renderer = ServiceLocator::get<IRendererService>();
    if(m_show_colliders) {
        for(const auto& body : m_rigidBodies) {
            body->DebugRender();
        }
    }
    if(m_show_joints) {
        for(const auto& joint : _joints) {
            joint->DebugRender();
        }
    }
    if(m_show_world_partition) {
        m_world_partition.DebugRender();
    }
    if(m_show_contacts) {
        renderer.SetModelMatrix(Matrix4::I);
    }
}

void PhysicsSystem::EndFrame() noexcept {
    //std::scoped_lock<std::mutex> lock(_cs);
    for(auto& body : m_rigidBodies) {
        body->Endframe();
    }
    for(auto* r : m_pending_removal) {
        m_rigidBodies.erase(std::remove_if(std::begin(m_rigidBodies), std::end(m_rigidBodies), [this, r](const RigidBody* b) { return b == r; }), std::end(m_rigidBodies));
        m_gravityFG.detach(r);
        m_dragFG.detach(r);
        for(auto&& fg : _forceGenerators) {
            fg->detach(r);
        }
        for(auto&& joint : _joints) {
            joint->Detach(r);
        }
    }
    m_pending_removal.clear();
    m_pending_removal.shrink_to_fit();
    _joints.erase(std::remove_if(std::begin(_joints), std::end(_joints), [](auto&& joint) -> bool { return joint->IsNotAttached(); }), std::end(_joints));
}

bool PhysicsSystem::ProcessSystemMessage([[maybe_unused]] const EngineMessage& msg) noexcept {
    return false;
}

void PhysicsSystem::AddObject(RigidBody* body) {
    m_pending_addition.push_back(body);
    //_world_partition.Add(body);
}

void PhysicsSystem::AddObjects(std::vector<RigidBody*> bodies) {
    m_pending_addition.reserve(m_rigidBodies.size() + bodies.size());
    m_pending_addition.insert(std::cend(m_pending_addition), std::cbegin(bodies), std::cend(bodies));
}

void PhysicsSystem::RemoveObject(RigidBody* body) {
    m_pending_removal.push_back(body);
}

void PhysicsSystem::RemoveObjects(std::vector<RigidBody*> bodies) {
    m_pending_removal.insert(std::cend(m_pending_removal), std::cbegin(bodies), std::cend(bodies));
}

void PhysicsSystem::RemoveAllObjects() noexcept {
    m_pending_removal.insert(std::cend(m_pending_removal), std::cbegin(m_rigidBodies), std::cend(m_rigidBodies));
}

void PhysicsSystem::RemoveAllObjectsImmediately() noexcept {
    m_rigidBodies.clear();
    m_rigidBodies.shrink_to_fit();
    m_gravityFG.detach_all();
    m_dragFG.detach_all();
    for(auto&& fg : _forceGenerators) {
        fg->detach_all();
    }
    _forceGenerators.clear();
    _forceGenerators.shrink_to_fit();
    for(auto&& joint : _joints) {
        joint->DetachAll();
    }
    _joints.clear();
    _joints.shrink_to_fit();
}

void PhysicsSystem::Debug_ShowCollision(bool show) {
    m_show_colliders = show;
}

void PhysicsSystem::Debug_ShowWorldPartition(bool show) {
    m_show_world_partition = show;
}

void PhysicsSystem::Debug_ShowContacts(bool show) {
    m_show_contacts = show;
}

void PhysicsSystem::Debug_ShowJoints(bool show) {
    m_show_joints = show;
}
