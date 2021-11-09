#pragma once

#include "Engine/Services/IService.hpp"

#include "Engine/Physics/Joint.hpp"
#include "Engine/Physics/ForceGenerator.hpp"
#include "Engine/Physics/PhysicsTypes.hpp"
#include "Engine/Physics/SpringJoint.hpp"
#include "Engine/Physics/RodJoint.hpp"
#include "Engine/Physics/CableJoint.hpp"

struct PhysicsSystemDesc;

class IPhysicsService : public IService {
public:
    virtual ~IPhysicsService() noexcept = default;

    virtual void AddObject(RigidBody* body) = 0;
    virtual void AddObjects(std::vector<RigidBody*> bodies) = 0;
    virtual void RemoveObject(RigidBody* body) = 0;
    virtual void RemoveObjects(std::vector<RigidBody*> bodies) = 0;
    virtual void RemoveAllObjects() noexcept = 0;
    virtual void RemoveAllObjectsImmediately() noexcept = 0;

    virtual void Enable(bool enable) = 0;
    virtual void SetGravity(const Vector2& new_gravity) = 0;
    virtual [[nodiscard]] Vector2 GetGravity() const noexcept = 0;
    virtual void SetDragCoefficients(const Vector2& k1k2) = 0;
    virtual void SetDragCoefficients(float linearCoefficient, float squareCoefficient) = 0;
    virtual [[nodiscard]] std::pair<float, float> GetDragCoefficients() const noexcept = 0;
    virtual [[nodiscard]] const PhysicsSystemDesc& GetWorldDescription() const noexcept = 0;
    virtual void SetWorldDescription(const PhysicsSystemDesc& new_desc) = 0;
    virtual void EnableGravity(bool isGravityEnabled) noexcept = 0;
    virtual void EnableDrag(bool isDragEnabled) noexcept = 0;
    virtual void EnablePhysics(bool isPhysicsEnabled) noexcept = 0;

    template<typename JointDefType>
    Joint* CreateJoint(const JointDefType& defType) noexcept;
    
    template<typename ForceGeneratorType>
    ForceGeneratorType* CreateForceGenerator();

    virtual [[nodiscard]] const std::vector<std::unique_ptr<Joint>>& Debug_GetJoints() const noexcept = 0;
    virtual [[nodiscard]] const std::vector<RigidBody*>& Debug_GetBodies() const noexcept = 0;

    virtual void Debug_ShowCollision(bool show) = 0;
    virtual void Debug_ShowWorldPartition(bool show) = 0;
    virtual void Debug_ShowContacts(bool show) = 0;
    virtual void Debug_ShowJoints(bool show) = 0;

protected:
    std::vector<std::unique_ptr<ForceGenerator>> _forceGenerators{};
    std::vector<std::unique_ptr<Joint>> _joints{};
private:
    
};

template<typename JointDefType>
Joint* IPhysicsService::CreateJoint(const JointDefType& defType) noexcept {
    static_assert(std::is_base_of_v<JointDef, JointDefType>, "CreateJoint received type not derived from Joint.");
    std::unique_ptr<Joint> newJoint{};
    if constexpr(std::is_same_v<JointDefType, SpringJointDef>) {
        newJoint.reset(new SpringJoint(defType));
    } else if constexpr(std::is_same_v<JointDefType, RodJointDef>) {
        newJoint.reset(new RodJoint(defType));
    } else if constexpr(std::is_same_v<JointDefType, CableJointDef>) {
        newJoint.reset(new CableJoint(defType));
    } else {
        static_assert(false, "CreateJoint received type not in if-else chain.");
    }
    auto* joint_ptr = newJoint.get();
    _joints.emplace_back(std::move(newJoint));
    return joint_ptr;
}

template<typename ForceGeneratorType>
ForceGeneratorType* IPhysicsService::CreateForceGenerator() {
    auto newFG = std::make_unique<ForceGeneratorType>();
    auto* new_fg_ptr = newFG.get();
    _forceGenerators.emplace_back(newFG);
    return new_fg_ptr;
}
