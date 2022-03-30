#include "Engine/Renderer/Camera3D.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include <algorithm>

Camera3D::Camera3D(const Camera2D& camera2D) noexcept
: trauma(camera2D.trauma)
, trauma_recovery_rate(camera2D.trauma_recovery_rate)
, m_aspect_ratio(camera2D.GetAspectRatio())
, m_far_distance(camera2D.GetFarDistance())
, m_near_distance(camera2D.GetNearDistance())
, m_position(camera2D.GetPosition())
, m_rotationRoll(camera2D.GetOrientation())
, m_view_matrix(camera2D.GetViewMatrix())
, m_projection_matrix(camera2D.GetProjectionMatrix())
, m_view_projection_matrix(camera2D.GetViewProjectionMatrix())
, m_inv_view_matrix(camera2D.GetInverseViewMatrix())
, m_inv_projection_matrix(camera2D.GetInverseProjectionMatrix())
, m_inv_view_projection_matrix(camera2D.GetInverseViewProjectionMatrix())
, m_rotation_matrix(camera2D.GetViewMatrix().GetRotation())
, m_rotation(m_rotation_matrix) {
    /* DO NOTHING */
}

Camera3D& Camera3D::operator=(const Camera2D& camera2D) noexcept {
    trauma = camera2D.trauma;
    trauma_recovery_rate = camera2D.trauma_recovery_rate;
    m_aspect_ratio = camera2D.GetAspectRatio();
    m_far_distance = camera2D.GetFarDistance();
    m_near_distance = (std::max)(0.01f, camera2D.GetNearDistance());
    m_position = Vector3{camera2D.GetPosition()};
    m_rotationRoll = camera2D.GetOrientation();
    m_view_matrix = camera2D.GetViewMatrix();
    m_projection_matrix = camera2D.GetProjectionMatrix();
    m_view_projection_matrix = camera2D.GetViewProjectionMatrix();
    m_inv_view_matrix = camera2D.GetInverseViewMatrix();
    m_inv_projection_matrix = camera2D.GetInverseProjectionMatrix();
    m_inv_view_projection_matrix = camera2D.GetInverseViewProjectionMatrix();
    m_rotation_matrix = camera2D.GetViewMatrix().GetRotation();
    m_rotation = Quaternion{m_rotation_matrix};
    return *this;
}

void Camera3D::SetupView(float fovVerticalDegrees, float aspectRatio /*= MathUtils::M_16_BY_9_RATIO*/, float nearDistance /*= 0.01f*/, float farDistance /*= 1.0f*/, const Vector3& worldUp /*= Vector3::Y_AXIS*/) noexcept {
    m_fov_vertical_degrees = fovVerticalDegrees;
    m_aspect_ratio = aspectRatio;
    m_near_distance = (std::max)(0.01f, nearDistance);
    m_far_distance = (std::max)(m_near_distance + 0.01f, farDistance);
    m_world_up = worldUp.GetNormalize();
    m_near_view_height = 2.0f * m_near_distance * std::tan(0.5f * m_fov_vertical_degrees);
    m_far_view_height = 2.0f * m_far_distance * std::tan(0.5f * m_fov_vertical_degrees);
    CalcRotationMatrix();
    CalcViewMatrix();
    CalcProjectionMatrix();
    CalcViewProjectionMatrix();
}

void Camera3D::CalcViewProjectionMatrix() noexcept {
    m_view_projection_matrix = Matrix4::MakeViewProjection(m_view_matrix, m_projection_matrix);
    m_inv_view_projection_matrix = Matrix4::CalculateInverse(m_view_projection_matrix);
}

void Camera3D::CalcProjectionMatrix() noexcept {
    m_projection_matrix = Matrix4::CreateDXPerspectiveProjection(m_fov_vertical_degrees, m_aspect_ratio, m_near_distance, m_far_distance);
    m_inv_projection_matrix = Matrix4::CalculateInverse(m_projection_matrix);
}

Matrix4 Camera3D::CreateBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    return Matrix4::MakeSRT(rotationMatrix, Matrix4::Create3DYRotationDegreesMatrix(180.0f), m_inv_view_matrix.GetRotation());
}

Matrix4 Camera3D::CreateReverseBillboardMatrix(const Matrix4& rotationMatrix) noexcept {
    return Matrix4::MakeRT(rotationMatrix, m_inv_view_matrix.GetRotation());
}

Vector3 Camera3D::GetEulerAngles() const noexcept {
    return Vector3{m_rotationPitch, m_rotationYaw, m_rotationRoll};
}

void Camera3D::CalcViewMatrix() noexcept {
    Matrix4 vT = Matrix4::CreateTranslationMatrix(-m_position);
    Matrix4 vQ = m_rotation_matrix;
    m_view_matrix = Matrix4::MakeRT(vT, vQ);
    m_inv_view_matrix = Matrix4::CalculateInverse(m_view_matrix);
}

void Camera3D::CalcRotationMatrix() noexcept {
    auto rot = Matrix4::CreateRotationYawRollPitchMatrixDegrees(m_rotationPitch, m_rotationYaw, m_rotationRoll);
    rot.OrthoNormalizeKIJ();
    const auto lookVector = rot.TransformDirection(Vector3::Z_Axis);

    const auto camPos = m_position;
    const auto camTarget = camPos + lookVector;
    m_rotation_matrix = Matrix4::CreateLookAtMatrix(camPos, camTarget, Vector3::Y_Axis);

    //float c_x_theta = MathUtils::CosDegrees(rotationPitch);
    //float s_x_theta = MathUtils::SinDegrees(rotationPitch);
    //Matrix4 Rx;
    //Rx.SetIBasis(Vector4(1.0f, 0.0f, 0.0f, 0.0f));
    //Rx.SetJBasis(Vector4(0.0f, c_x_theta, s_x_theta, 0.0f));
    //Rx.SetKBasis(Vector4(0.0f, -s_x_theta, c_x_theta, 0.0f));

    //float c_y_theta = MathUtils::CosDegrees(rotationYaw);
    //float s_y_theta = MathUtils::SinDegrees(rotationYaw);
    //Matrix4 Ry;
    //Ry.SetIBasis(Vector4(c_y_theta, 0.0f, -s_y_theta, 0.0f));
    //Ry.SetJBasis(Vector4(0.0f, 1.0f, 0.0f, 0.0f));
    //Ry.SetKBasis(Vector4(s_y_theta, 0.0f, c_y_theta, 0.0f));

    //float c_z_theta = MathUtils::CosDegrees(rotationRoll);
    //float s_z_theta = MathUtils::SinDegrees(rotationRoll);
    //Matrix4 Rz;
    //Rz.SetIBasis(Vector4(c_z_theta, s_z_theta, 0.0f, 0.0f));
    //Rz.SetJBasis(Vector4(-s_z_theta, c_z_theta, 0.0f, 0.0f));
    //Rz.SetKBasis(Vector4(0.0f, 0.0f, 1.0f, 0.0f));

    ////Matrix4 R = Matrix4::MakeSRT(Ry, Rx, Rz);
    //Matrix4 R = Matrix4::MakeSRT(Rz, Rx, Ry);
    //rotation_matrix = R;
}

void Camera3D::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    trauma -= trauma_recovery_rate * deltaSeconds.count();
}

const Vector3& Camera3D::GetPosition() const noexcept {
    return m_position;
}

void Camera3D::SetPosition(const Vector3& newPosition) noexcept {
    m_position = newPosition;
}

void Camera3D::SetPosition(float x, float y, float z) noexcept {
    SetPosition(Vector3{x, y, z});
}

void Camera3D::SetPosition(const Vector2& newPosition) noexcept {
    SetPosition(newPosition.x, newPosition.y, 0.0f);
}

void Camera3D::SetPosition(float x, float y) noexcept {
    SetPosition(Vector2{x, y});
}

void Camera3D::Translate(const Vector3& displacement) noexcept {
    m_position += displacement;
}

void Camera3D::Translate(float x, float y, float z) noexcept {
    Translate(Vector3{x, y, z});
}

void Camera3D::Translate(const Vector2& displacement) noexcept {
    Translate(displacement.x, displacement.y, 0.0f);
}

void Camera3D::Translate(float x, float y) noexcept {
    Translate(Vector2{x, y});
}

float Camera3D::CalcFovYDegrees() const noexcept {
    return m_fov_vertical_degrees;
}

float Camera3D::CalcFovXDegrees() const noexcept {
    const auto half_width = 0.5f * CalcNearViewWidth();
    return 2.0f * std::atan(half_width / m_near_distance);
}

float Camera3D::CalcNearViewWidth() const noexcept {
    return m_aspect_ratio * m_near_view_height;
}

float Camera3D::CalcNearViewHeight() const noexcept {
    return m_near_view_height;
}

float Camera3D::CalcFarViewWidth() const noexcept {
    return m_aspect_ratio * m_far_view_height;
}

float Camera3D::CalcFarViewHeight() const noexcept {
    return m_far_view_height;
}

float Camera3D::GetAspectRatio() const noexcept {
    return m_aspect_ratio;
}

float Camera3D::GetInverseAspectRatio() const noexcept {
    return 1.0f / m_aspect_ratio;
}

float Camera3D::GetNearDistance() const noexcept {
    return m_near_distance;
}

float Camera3D::GetFarDistance() const noexcept {
    return m_far_distance;
}

const Matrix4& Camera3D::GetRotationMatrix() const noexcept {
    return m_rotation_matrix;
}

const Matrix4& Camera3D::GetViewMatrix() const noexcept {
    return m_view_matrix;
}

const Matrix4& Camera3D::GetProjectionMatrix() const noexcept {
    return m_projection_matrix;
}

const Matrix4& Camera3D::GetViewProjectionMatrix() const noexcept {
    return m_view_projection_matrix;
}

const Matrix4& Camera3D::GetInverseViewMatrix() const noexcept {
    return m_inv_view_matrix;
}

const Matrix4& Camera3D::GetInverseProjectionMatrix() const noexcept {
    return m_inv_projection_matrix;
}

const Matrix4& Camera3D::GetInverseViewProjectionMatrix() const noexcept {
    return m_inv_view_projection_matrix;
}

void Camera3D::SetEulerAngles(const Vector3& eulerAngles) noexcept {
    SetEulerAnglesDegrees(Vector3{MathUtils::ConvertRadiansToDegrees(eulerAngles.x), MathUtils::ConvertRadiansToDegrees(eulerAngles.y), MathUtils::ConvertRadiansToDegrees(eulerAngles.z)});
}

void Camera3D::SetEulerAnglesDegrees(const Vector3& eulerAnglesDegrees) noexcept {
    m_rotationPitch = std::clamp(eulerAnglesDegrees.x, -89.0f, 89.0f);
    m_rotationYaw = MathUtils::Wrap(eulerAnglesDegrees.y, 0.0f, 360.0f);
    m_rotationRoll = MathUtils::Wrap(eulerAnglesDegrees.z, 0.0f, 360.f);
}

void Camera3D::SetForwardFromTarget(const Vector3& lookAtPosition) noexcept {
    Vector3 forward = (lookAtPosition - m_position).GetNormalize();
    Vector3 right = MathUtils::CrossProduct(m_world_up.GetNormalize(), forward).GetNormalize();
    Vector3 up = MathUtils::CrossProduct(forward, right).GetNormalize();
    Matrix4 m;
    m.SetIBasis(Vector4(right, 0.0f));
    m.SetJBasis(Vector4(up, 0.0f));
    m.SetKBasis(Vector4(forward, 0.0f));
    m.OrthoNormalizeIJK();
    m_rotation = Quaternion(m);
    const auto eulerangles = m_rotation.CalcEulerAnglesDegrees();
    m_rotationPitch = std::clamp(eulerangles.x, -89.0f, 89.0f);
    m_rotationYaw = MathUtils::Wrap(eulerangles.y, 0.0f, 360.0f);
    m_rotationRoll = MathUtils::Wrap(eulerangles.z, 0.0f, 360.0f);
}

void Camera3D::RotateBy(const Vector3& eulerAngles) noexcept {
    const auto p = MathUtils::ConvertRadiansToDegrees(eulerAngles.x);
    const auto y = MathUtils::ConvertRadiansToDegrees(eulerAngles.y);
    const auto r = MathUtils::ConvertRadiansToDegrees(eulerAngles.z);
    RotateDegreesBy(Vector3{p, y, r});
}

void Camera3D::RotateDegreesBy(const Vector3& eulerAnglesDegrees) noexcept {
    m_rotationPitch += eulerAnglesDegrees.x;
    m_rotationPitch = std::clamp(m_rotationPitch, -89.9f, 89.9f);
    m_rotationYaw += -eulerAnglesDegrees.y;
    m_rotationYaw = MathUtils::Wrap(m_rotationYaw, 0.0f, 360.0f);
    m_rotationRoll += eulerAnglesDegrees.z;
    m_rotationRoll = MathUtils::Wrap(m_rotationRoll, 0.0f, 360.0f);
    m_rotation = Quaternion::CreateFromEulerAngles(m_rotationPitch, m_rotationYaw, m_rotationRoll, true);
}

void Camera3D::RotatePitchDegreesBy(float angleDegrees) noexcept {
    m_rotationPitch += angleDegrees;
    m_rotationPitch = std::clamp(m_rotationPitch, -89.0f, 89.0f);
}

void Camera3D::RotateYawDegreesBy(float angleDegrees) noexcept {
    m_rotationYaw += angleDegrees;
    m_rotationYaw = MathUtils::Wrap(m_rotationYaw, 0.0f, 360.0f);
}

void Camera3D::RotateRollDegreesBy(float angleDegrees) noexcept {
    m_rotationRoll += angleDegrees;
    m_rotationRoll = MathUtils::Wrap(m_rotationRoll, 0.0f, 360.0f);
}

void Camera3D::RotatePitchBy(float angleRadians) noexcept {
    RotatePitchDegreesBy(MathUtils::ConvertRadiansToDegrees(angleRadians));
}

void Camera3D::RotateYawBy(float angleRadians) noexcept {
    RotateYawDegreesBy(MathUtils::ConvertRadiansToDegrees(angleRadians));
}

void Camera3D::RotateRollBy(float angleRadians) noexcept {
    RotateRollDegreesBy(MathUtils::ConvertRadiansToDegrees(angleRadians));
}

void Camera3D::Rotate(const Vector3& axis, const float angle) noexcept {
    const auto degrees = MathUtils::ConvertRadiansToDegrees(angle);
    RotateDegrees(axis, degrees);
}

void Camera3D::RotateDegrees(const Vector3& axis, const float angleDegrees) noexcept {
    m_rotation = Quaternion::CreateFromAxisAngle(axis, angleDegrees);
    const auto eulerangles = m_rotation.CalcEulerAnglesDegrees();
    m_rotationPitch = std::clamp(eulerangles.x, -89.0f, 89.0f);
    m_rotationYaw = MathUtils::Wrap(eulerangles.y, 0.0f, 360.0f);
    m_rotationRoll = MathUtils::Wrap(eulerangles.z, 0.0f, 360.0f);
}

Vector3 Camera3D::GetRight() const noexcept {
    const auto forward = GetForward();
    const auto right = MathUtils::CrossProduct(m_world_up, forward);
    return right.GetNormalize();
}

Vector3 Camera3D::GetRightXY() const noexcept {
    const auto forward = GetForwardXY();
    return Vector3{forward.y, forward.x, 0.0f};
}

Vector3 Camera3D::GetRightXZ() const noexcept {
    const auto forward = GetForwardXZ();
    return Vector3{-forward.y, 0.0f, forward.x};
}

Vector3 Camera3D::GetUp() const noexcept {
    const auto forward = GetForward();
    const auto right = GetRight();
    const auto up = MathUtils::CrossProduct(forward, right);
    return up.GetNormalize();
}

Vector3 Camera3D::GetUpXY() const noexcept {
    const auto forward = GetForwardXY();
    const auto right = GetRightXY();
    const auto up = MathUtils::CrossProduct(forward, right);
    return up.GetNormalize();
}

Vector3 Camera3D::GetUpXZ() const noexcept {
    const auto forward = GetForwardXZ();
    const auto right = GetRightXZ();
    const auto up = MathUtils::CrossProduct(forward, right);
    return up.GetNormalize();
}

Vector3 Camera3D::GetForward() const noexcept {
    return Vector3::Z_Axis * Matrix4{m_rotation};
    //float cos_yaw = MathUtils::CosDegrees(rotationYaw);
    //float cos_pitch = MathUtils::CosDegrees(rotationPitch);

    //float sin_yaw = MathUtils::SinDegrees(rotationYaw);
    //float sin_pitch = MathUtils::SinDegrees(rotationPitch);

    //return Vector3(-sin_yaw * cos_pitch, sin_pitch, cos_yaw * cos_pitch);
    //return Vector3(sin_yaw * cos_pitch, sin_pitch, cos_yaw * cos_pitch);
}

Vector3 Camera3D::GetForwardXY() const noexcept {
    const auto cos_yaw = MathUtils::CosDegrees(m_rotationYaw);
    const auto sin_yaw = MathUtils::SinDegrees(m_rotationYaw);

    return Vector3(cos_yaw, sin_yaw, 0.0f);
}

Vector3 Camera3D::GetForwardXZ() const noexcept {
    const auto cos_yaw = MathUtils::CosDegrees(m_rotationYaw);
    const auto sin_yaw = MathUtils::SinDegrees(m_rotationYaw);

    return Vector3(-sin_yaw, 0.0f, cos_yaw);
}

float Camera3D::GetYawDegrees() const noexcept {
    return m_rotationYaw;
}

float Camera3D::GetPitchDegrees() const noexcept {
    return m_rotationPitch;
}

float Camera3D::GetRollDegrees() const noexcept {
    return m_rotationRoll;
}

float Camera3D::GetYaw() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetYawDegrees());
}

float Camera3D::GetPitch() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetPitchDegrees());
}

float Camera3D::GetRoll() const noexcept {
    return MathUtils::ConvertDegreesToRadians(GetRollDegrees());
}
