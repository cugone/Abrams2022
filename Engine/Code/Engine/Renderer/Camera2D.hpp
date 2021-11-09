#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Renderer/RenderTargetStack.hpp"

class Camera2D {
public:
    Camera2D() = default;
    Camera2D(const Camera2D& camera2D) noexcept = default;
    Camera2D(Camera2D&& camera2D) noexcept = default;
    Camera2D& operator=(const Camera2D& camera2D) noexcept = default;
    Camera2D& operator=(Camera2D&& camera2D) noexcept = default;
    ~Camera2D() = default;

    void SetupView(const Vector2& leftBottom, const Vector2& rightTop, const Vector2& nearFar = Vector2(0.0f, 1.0f), float aspectRatio = MathUtils::M_16_BY_9_RATIO) noexcept;

    void Update(TimeUtils::FPSeconds deltaSeconds) noexcept;

    [[nodiscard]] const Vector2& GetPosition() const noexcept;
    void SetPosition(const Vector3& newPosition) noexcept;
    void SetPosition(const Vector2& newPosition) noexcept;
    void Translate(const Vector3& displacement) noexcept;
    void Translate(const Vector2& displacement) noexcept;
    [[nodiscard]] float GetOrientationDegrees() const noexcept;
    void SetOrientationDegrees(float newAngleDegrees) noexcept;
    void ApplyOrientationDegrees(float addAngleDegrees) noexcept;
    [[nodiscard]] float GetOrientation() const noexcept;
    void SetOrientation(float newAngleRadians) noexcept;
    void ApplyOrientation(float addAngleRadians) noexcept;
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

    [[nodiscard]] const RenderTargetStack::Node& GetRenderTarget() const noexcept;
    [[nodiscard]] RenderTargetStack::Node& GetRenderTarget() noexcept;

    [[nodiscard]] Vector2 GetViewDimensions() const noexcept;
    [[nodiscard]] float GetViewHeight() const noexcept;
    [[nodiscard]] float GetViewWidth() const noexcept;

    [[nodiscard]] float GetShake() const noexcept;

    [[nodiscard]] static Vector2 WindowToWorldPoint(const Camera2D& camera, const Vector2& screenPoint) noexcept;
    [[nodiscard]] Vector2 WindowToWorldPoint(const Vector2& screenPoint) const noexcept;

    float trauma = 0.0f;
    float trauma_recovery_rate = 1.0f;
    Vector2 position = Vector2::Zero;
    float orientation_degrees = 0.0f;

protected:
private:
    void CalcViewMatrix() noexcept;
    void CalcProjectionMatrix() noexcept;
    void CalcViewProjectionMatrix() noexcept;

    Matrix4 view_matrix = Matrix4::I;
    Matrix4 projection_matrix = Matrix4::I;
    Matrix4 view_projection_matrix = Matrix4::I;

    Matrix4 inv_view_matrix = Matrix4::I;
    Matrix4 inv_projection_matrix = Matrix4::I;
    Matrix4 inv_view_projection_matrix = Matrix4::I;

    Vector2 leftBottom_view = Vector2{-1.0f, 1.0f};
    Vector2 rightTop_view = Vector2{1.0f, -1.0f};
    Vector2 nearFar_distance = Vector2{0.0f, 1.0f};
    RenderTargetStack::Node _render_target{};
    float aspect_ratio = MathUtils::M_16_BY_9_RATIO;
};