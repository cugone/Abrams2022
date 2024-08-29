#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Physics/CableJoint.hpp"
#include "Engine/Physics/DragForceGenerator.hpp"
#include "Engine/Physics/ForceGenerator.hpp"
#include "Engine/Physics/GravityForceGenerator.hpp"
#include "Engine/Physics/Joint.hpp"
#include "Engine/Physics/PhysicsTypes.hpp"
#include "Engine/Physics/RigidBody.hpp"
#include "Engine/Physics/RodJoint.hpp"
#include "Engine/Physics/SpringJoint.hpp"
#include "Engine/Physics/QuadTree.hpp"
#include "Engine/Profiling/ProfileLogScope.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Services/IPhysicsService.hpp"

#include <atomic>
#include <condition_variable>
#include <queue>
#include <set>
#include <thread>
#include <utility>
#include <vector>

class Joint;

class PhysicsSystem : public EngineSubsystem, public IPhysicsService {
public:
    explicit PhysicsSystem(const PhysicsSystemDesc& desc = PhysicsSystemDesc{});
    virtual ~PhysicsSystem() noexcept;

    /************************************************************************/
    /* BEGIN ENGINE SUBSYSTEM INTERFACE                                     */
    /************************************************************************/

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    [[nodiscard]] bool ProcessSystemMessage([[maybe_unused]] const EngineMessage& msg) noexcept override;

    /************************************************************************/
    /* END ENGINE SUBSYSTEM INTERFACE                                       */
    /************************************************************************/


    /************************************************************************/
    /* BEGIN SERVICE LOCATOR INTERFACE                                      */
    /************************************************************************/

    void AddObject(RigidBody* body) override;
    void AddObjects(std::vector<RigidBody*> bodies) override;
    void RemoveObject(RigidBody* body) override;
    void RemoveObjects(std::vector<RigidBody*> bodies) override;
    void RemoveAllObjects() noexcept override;
    void RemoveAllObjectsImmediately() noexcept override;

    void Enable(bool enable) override;
    void SetGravity(const Vector2& new_gravity) override;
    [[nodiscard]] Vector2 GetGravity() const noexcept override;
    void SetDragCoefficients(const Vector2& k1k2) override;
    void SetDragCoefficients(float linearCoefficient, float squareCoefficient) override;
    [[nodiscard]] std::pair<float, float> GetDragCoefficients() const noexcept override;
    [[nodiscard]] const PhysicsSystemDesc& GetWorldDescription() const noexcept override;
    void SetWorldDescription(const PhysicsSystemDesc& new_desc) override;
    void EnableGravity(bool isGravityEnabled) noexcept override;
    void EnableDrag(bool isDragEnabled) noexcept override;
    void EnablePhysics(bool isPhysicsEnabled) noexcept override;

    [[nodiscard]] const std::vector<std::unique_ptr<Joint>>& Debug_GetJoints() const noexcept override;
    [[nodiscard]] const std::vector<RigidBody*>& Debug_GetBodies() const noexcept override;

    void Debug_ShowCollision(bool show) override;
    void Debug_ShowWorldPartition(bool show) override;
    void Debug_ShowContacts(bool show) override;
    void Debug_ShowJoints(bool show) override;

    /************************************************************************/
    /* END SERVICE LOCATOR INTERFACE                                        */
    /************************************************************************/

protected:
private:
    void UpdateBodiesInBounds(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ApplyCustomAndJointForces(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ApplyGravityAndDrag(TimeUtils::FPSeconds deltaSeconds) noexcept;
    [[nodiscard]] std::vector<RigidBody*> BroadPhaseCollision(const AABB2& query_area) noexcept;

    using CollisionDataSet = std::set<CollisionData>;
    template<typename CollisionDetectionFunction, typename CollisionResolutionFunction>
    [[nodiscard]] CollisionDataSet NarrowPhaseCollision(const std::vector<RigidBody*>& potential_collisions, CollisionDetectionFunction&& cd, CollisionResolutionFunction&& cr) noexcept;

    void SolveCollision(const CollisionDataSet& actual_collisions) noexcept;
    void SolveConstraints() const noexcept;
    void SolvePositionConstraints() const noexcept;
    void SolveVelocityConstraints() const noexcept;

    bool m_is_running = false;
    std::deque<CollisionData> m_contacts{};
    std::vector<RigidBody*> m_pending_removal{};
    std::vector<RigidBody*> m_pending_addition{};
    GravityForceGenerator m_gravityFG{Vector2::Zero};
    DragForceGenerator m_dragFG{Vector2::Zero};
    QuadTree<RigidBody> m_world_partition{};
    TimeUtils::FPSeconds m_deltaSeconds = TimeUtils::FPSeconds::zero();
    TimeUtils::FPSeconds m_accumulatedTime = TimeUtils::FPSeconds::zero();
    TimeUtils::FPFrames m_targetFrameRate = TimeUtils::FPFrames{1};
    bool m_show_colliders = false;
    bool m_show_object_bounds = false;
    bool m_show_world_partition = false;
    bool m_show_contacts = false;
    bool m_show_joints = false;
};

template<typename CollisionDetectionFunction, typename CollisionResolutionFunction>
PhysicsSystem::CollisionDataSet PhysicsSystem::NarrowPhaseCollision(const std::vector<RigidBody*>& potential_collisions, CollisionDetectionFunction&& cd, CollisionResolutionFunction&& cr) noexcept {
    CollisionDataSet result;
    if(potential_collisions.size() < 2) {
        m_contacts.clear();
        return {};
    }
    for(auto iter_a = std::begin(potential_collisions); iter_a != std::end(potential_collisions); ++iter_a) {
        for(auto iter_b = iter_a + 1; iter_b != std::end(potential_collisions); ++iter_b) {
            auto* const cur_body = *iter_a;
            auto* const next_body = *iter_b;
            if(cur_body == next_body) {
                continue;
            }
            const auto cdResult = std::invoke(cd, *cur_body->GetCollider(), *next_body->GetCollider());
            if(cdResult.collides) {
                const auto crResult = std::invoke(cr, cdResult, *cur_body->GetCollider(), *next_body->GetCollider());
                const auto contact = CollisionData{cur_body, next_body, crResult.distance, crResult.normal};
                if(const auto&& [_, was_inserted] = result.insert(contact); was_inserted) {
                    while(m_contacts.size() >= 10) {
                        m_contacts.pop_front();
                    }
                    m_contacts.push_back(contact);
                }
            }
        }
    }
    return result;
}
