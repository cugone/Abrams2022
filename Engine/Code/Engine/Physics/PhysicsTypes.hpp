#pragma once

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"

#include <vector>

class RigidBody;

struct PhysicsMaterial {
    float friction = 0.0f;      //0.7f; //Range: [0.0,1.0]; How quickly an object comes to rest during a contact. Values closer to 1.0 cause resting contacts to lose velocity faster.
    float restitution = 0.0f;   //0.3f; //Range: [-1.0f, 1.0f]; The bouncyness of a material. Negative values cause an object to gain velocity after a collision.
    float density = 1.0f;       //Affect mass calculation for "bigger" objects.
    float massExponent = 0.01f; //Raise final mass calculation to this exponent.
};

struct PhysicsDesc {
    float mass = 1.0f; //How "heavy" an object is. Expressed in Kilograms. Cannot be lower than 0.001f;
    float maxAngularSpeed = 1000.0f;
    float linearDamping = 1.0f;
    float angularDamping = 1.0f;
    bool enableGravity = true; //Should gravity be applied.
    bool enableDrag = true;    //Should drag be applied.
    bool enablePhysics = true; //Should object be subject to physics calculations.
    bool startAwake = true;    //Should the object be awake on creation.
};

struct Velocity;
struct Acceleration;

struct Position {
    Position() = default;
    Position(const Position& p) = default;
    Position(Position&& p) = default;
    Position& operator=(const Position& p) = default;
    Position& operator=(Position&& p) = default;
    Position(float x, float y) noexcept;
    Position(Vector2 value) noexcept;
    explicit Position(Velocity) noexcept = delete;
    explicit Position(Acceleration) noexcept = delete;
    operator Vector2 const();
    operator Vector2();
    [[nodiscard]] Vector2 Get() const noexcept;
    void Set(Vector2 value) noexcept;

private:
    Vector2 m_value{};
};

struct Velocity {
    Velocity() = default;
    Velocity(const Velocity& p) = default;
    Velocity(Velocity&& p) = default;
    Velocity& operator=(const Velocity& p) = default;
    Velocity& operator=(Velocity&& p) = default;
    Velocity(float x, float y) noexcept;
    Velocity(Vector2 value) noexcept;
    explicit Velocity(Acceleration) noexcept = delete;
    explicit Velocity(Position) noexcept = delete;
    operator Vector2 const();
    operator Vector2();
    [[nodiscard]] Vector2 Get() const noexcept;
    void Set(Vector2 value) noexcept;

private:
    Vector2 m_value{};
};

struct Acceleration {
    Acceleration() = default;
    Acceleration(const Acceleration& p) = default;
    Acceleration(Acceleration&& p) = default;
    Acceleration& operator=(const Acceleration& p) = default;
    Acceleration& operator=(Acceleration&& p) = default;
    Acceleration(float x, float y) noexcept;
    Acceleration(Vector2 value) noexcept;
    explicit Acceleration(Position) noexcept = delete;
    explicit Acceleration(Velocity) noexcept = delete;
    operator Vector2 const();
    operator Vector2();
    [[nodiscard]] Vector2 Get() const noexcept;
    void Set(Vector2 value) noexcept;

private:
    Vector2 m_value{};
};

struct GJKResult {
    bool collides{false};
    std::vector<Vector3> simplex{};
};

struct EPAResult {
    float distance{0.0f};
    Vector3 normal{};
};

struct CollisionData {
    RigidBody* const a = nullptr;
    RigidBody* const b = nullptr;
    float distance = 0.0f;
    Vector3 normal{};
    CollisionData(RigidBody* const a, RigidBody* const b, float distance, const Vector3& normal);
    [[nodiscard]] bool operator<(const CollisionData& rhs) const noexcept;
    [[nodiscard]] bool operator==(const CollisionData& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const CollisionData& rhs) const noexcept;
};
