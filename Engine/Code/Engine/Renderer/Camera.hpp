#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Quaternion.hpp"
#include "Engine/Math/Rotator.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Renderer/RenderTargetStack.hpp"

// clang-format off
enum class ProjectionMode {
    Orthographic
    , Perspective
};
// clang-format on

class Camera {
public:
    void SetProjectionMode(ProjectionMode newProjectionMode) noexcept;
    [[nodiscard]] ProjectionMode GetProjectionMode() const noexcept;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;

    [[nodiscard]] const Vector3& GetPosition() const noexcept;
    void SetPosition(const Vector3& newPosition) noexcept;
    void SetPosition(const Vector2& newPosition) noexcept;

    void Translate(const Vector3& displacement) noexcept;
    void Translate(const Vector2& displacement) noexcept;

    [[nodiscard]] float GetAspectRatio() const noexcept;
    [[nodiscard]] float GetInverseAspectRatio() const noexcept;
    [[nodiscard]] float GetNearDistance() const noexcept;
    [[nodiscard]] float GetFarDistance() const noexcept;

    [[nodiscard]] const Matrix4& GetViewMatrix() const noexcept;
    [[nodiscard]] const Matrix4& GetProjectionMatrix() const noexcept;
    [[nodiscard]] const Matrix4& GetViewProjectionMatrix() const noexcept;

    [[nodiscard]] const Matrix4& GetInverseViewMatrix() const noexcept;
    [[nodiscard]] const Matrix4& GetInverseProjectionMatrix() const noexcept;
    [[nodiscard]] const Matrix4& GetInverseViewProjectionMatrix() const noexcept;

    [[nodiscard]] float GetShake() const noexcept;

    [[nodiscard]] const Matrix4& GetRotationMatrix() const noexcept;
    [[nodiscard]] Matrix4 CreateBillboardMatrix(const Matrix4& rotationMatrix) noexcept;
    [[nodiscard]] Matrix4 CreateReverseBillboardMatrix(const Matrix4& rotationMatrix) noexcept;

    [[nodiscard]] Rotator GetRotation() const noexcept;

    [[nodiscard]] Vector3 GetEulerAngles() const noexcept;
    [[nodiscard]] void SetEulerAnglesDegrees(const Vector3& eulerAnglesDegrees) noexcept;
    [[nodiscard]] void SetForwardFromTarget(const Vector3& lookAtPosition) noexcept;

    [[nodiscard]] Vector3 GetRight() const noexcept;
    [[nodiscard]] Vector3 GetUp() const noexcept;
    [[nodiscard]] Vector3 GetForward() const noexcept;

    [[nodiscard]] float GetYaw() const noexcept;
    [[nodiscard]] float GetPitch() const noexcept;
    [[nodiscard]] float GetRoll() const noexcept;

    float trauma = 0.0f;
    float trauma_recovery_rate = 1.0f;

protected:
private:
    void RecalculateProjectionMatrix() noexcept;
    void RecalculateViewMatrix() noexcept;
    void RecalculateViewProjectionMatrix() noexcept;

    float m_aspect_ratio = MathUtils::M_16_BY_9_RATIO;
    float m_fov = 60.0f;
    float m_ortho_width = 8.0f;
    float m_near_distance = 0.01f;
    float m_far_distance = 1.0f;
    Vector3 m_position = Vector3::Zero;

    Matrix4 m_view_matrix = Matrix4::I;
    Matrix4 m_rotation_matrix = Matrix4::I;
    Matrix4 m_projection_matrix = Matrix4::I;
    Matrix4 m_view_projection_matrix = Matrix4::I;
    Matrix4 m_inv_view_matrix = Matrix4::I;
    Matrix4 m_inv_projection_matrix = Matrix4::I;
    Matrix4 m_inv_view_projection_matrix = Matrix4::I;
    Rotator m_rotation{};
    ProjectionMode m_projection_mode = ProjectionMode::Orthographic;
};
