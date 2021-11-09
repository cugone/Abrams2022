#pragma once

#include "Engine/Math/Quaternion.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

#include <array>
#include <string>

class AABB3;
class Camera3D;

class Matrix4 {
public:
    static const Matrix4 I;

    [[nodiscard]] static Matrix4 CreateTranslationMatrix(const Vector2& position) noexcept;
    [[nodiscard]] static Matrix4 CreateTranslationMatrix(const Vector3& position) noexcept;

    [[nodiscard]] static Matrix4 Create2DRotationDegreesMatrix(float angleDegrees) noexcept;
    [[nodiscard]] static Matrix4 Create3DXRotationDegreesMatrix(float angleDegrees) noexcept;
    [[nodiscard]] static Matrix4 Create3DYRotationDegreesMatrix(float angleDegrees) noexcept;
    [[nodiscard]] static Matrix4 Create3DZRotationDegreesMatrix(float angleDegrees) noexcept;

    [[nodiscard]] static Matrix4 CreateZYXRotationMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateZYXRotationMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateZYXRotationMatrixDegrees(const Vector3& anglesDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateZYXRotationMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollYawPitchMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollYawPitchMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollYawPitchMatrixDegrees(const Vector3& pitchYawRollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollYawPitchMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;

    [[nodiscard]] static Matrix4 CreateZXYRotationMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateZXYRotationMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateZXYRotationMatrixDegrees(const Vector3& anglesDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateZXYRotationMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollPitchYawMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollPitchYawMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollPitchYawMatrixDegrees(const Vector3& pitchYawRollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationRollPitchYawMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;

    [[nodiscard]] static Matrix4 CreateYZXRotationMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateYZXRotationMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateYZXRotationMatrixDegrees(const Vector3& anglesDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateYZXRotationMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationYawRollPitchMatrix(const Vector3& pitchYawRollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationYawRollPitchMatrix(float pitchRadians, float yawRadians, float rollRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationYawRollPitchMatrixDegrees(const Vector3& pitchYawRollDegrees) noexcept;
    [[nodiscard]] static Matrix4 CreateRotationYawRollPitchMatrixDegrees(float pitchDegrees, float yawDegrees, float rollDegrees) noexcept;

    [[nodiscard]] static Matrix4 Create2DRotationMatrix(float angleRadians) noexcept;
    [[nodiscard]] static Matrix4 Create3DXRotationMatrix(float angleRadians) noexcept;
    [[nodiscard]] static Matrix4 Create3DYRotationMatrix(float angleRadians) noexcept;
    [[nodiscard]] static Matrix4 Create3DZRotationMatrix(float angleRadians) noexcept;
    [[nodiscard]] static Matrix4 CreateScaleMatrix(float scale) noexcept;
    [[nodiscard]] static Matrix4 CreateScaleMatrix(const Vector2& scale) noexcept;
    [[nodiscard]] static Matrix4 CreateScaleMatrix(const Vector3& scale) noexcept;
    [[nodiscard]] static Matrix4 CreateTransposeMatrix(const Matrix4& mat) noexcept;
    [[nodiscard]] static Matrix4 CreatePerspectiveProjectionMatrix(float top, float bottom, float right, float left, float nearZ, float farZ) noexcept;
    [[nodiscard]] static Matrix4 CreateHPerspectiveProjectionMatrix(float fov, float aspect_ratio, float nearZ, float farZ) noexcept;
    [[nodiscard]] static Matrix4 CreateVPerspectiveProjectionMatrix(float fov, float aspect_ratio, float nearZ, float farZ) noexcept;
    [[nodiscard]] static Matrix4 CreateDXOrthographicProjection(float nx, float fx, float ny, float fy, float nz, float fz) noexcept;
    [[nodiscard]] static Matrix4 CreateDXOrthographicProjection(const AABB3& extents) noexcept;
    [[nodiscard]] static Matrix4 CreateDXPerspectiveProjection(float vfovDegrees, float aspect, float nz, float fz) noexcept;
    [[nodiscard]] static Matrix4 CreateOrthographicProjectionMatrix(float top, float bottom, float right, float left, float nearZ, float farZ) noexcept;
    [[nodiscard]] static Matrix4 CreateLookAtMatrix(const Vector3& cameraPosition, const Vector3& lookAt, const Vector3& up) noexcept;
    [[nodiscard]] static Matrix4 CalculateChangeOfBasisMatrix(const Matrix4& output_basis, const Matrix4& input_basis = Matrix4::I) noexcept;

    [[nodiscard]] static Matrix4 MakeSRT(const Matrix4& S, const Matrix4& R, const Matrix4& T) noexcept;
    [[nodiscard]] static Matrix4 MakeRT(const Matrix4& R, const Matrix4& T) noexcept;
    [[nodiscard]] static Matrix4 MakeViewProjection(const Matrix4& viewMatrix, const Matrix4& projectionMatrix) noexcept;

    Matrix4() = default;
    explicit Matrix4(const std::string& value) noexcept;
    Matrix4(const Matrix4& other) = default;
    Matrix4(Matrix4&& other) = default;
    Matrix4& operator=(Matrix4&& rhs) = default;
    Matrix4& operator=(const Matrix4& rhs) = default;
    ~Matrix4() = default;

    explicit Matrix4(const Quaternion& q) noexcept;
    explicit Matrix4(const Vector2& iBasis, const Vector2& jBasis, const Vector2& translation = Vector2::Zero) noexcept;
    explicit Matrix4(const Vector3& iBasis, const Vector3& jBasis, const Vector3& kBasis, const Vector3& translation = Vector3::Zero) noexcept;
    explicit Matrix4(const Vector4& iBasis, const Vector4& jBasis, const Vector4& kBasis, const Vector4& translation = Vector4::Zero_XYZ_One_W) noexcept;
    explicit Matrix4(const float* arrayOfFloats) noexcept;

    void Identity() noexcept;
    void Transpose() noexcept;
    [[nodiscard]] float CalculateTrace() const noexcept;
    [[nodiscard]] float CalculateTrace() noexcept;
    [[nodiscard]] Vector4 GetDiagonal() const noexcept;
    [[nodiscard]] static Vector4 GetDiagonal(const Matrix4& mat) noexcept;

    [[nodiscard]] bool IsInvertable() const noexcept;
    [[nodiscard]] bool IsSingular() const noexcept;

    void CalculateInverse() noexcept;
    [[nodiscard]] static float CalculateDeterminant(const Matrix4& mat) noexcept;
    [[nodiscard]] float CalculateDeterminant() const noexcept;
    [[nodiscard]] float CalculateDeterminant() noexcept;
    [[nodiscard]] static Matrix4 CalculateInverse(const Matrix4& mat) noexcept;

    void OrthoNormalizeIKJ() noexcept;
    void OrthoNormalizeIJK() noexcept;
    void OrthoNormalizeKIJ() noexcept;

    void Translate(const Vector2& translation2D) noexcept;
    void Translate(const Vector3& translation3D) noexcept;

    void Scale(float scale) noexcept;
    void Scale(const Vector2& scale) noexcept;
    void Scale(const Vector3& scale) noexcept;
    void Scale(const Vector4& scale) noexcept;

    void Rotate3DXDegrees(float degrees) noexcept;
    void Rotate3DYDegrees(float degrees) noexcept;
    void Rotate3DZDegrees(float degrees) noexcept;
    void Rotate2DDegrees(float degrees) noexcept;

    void Rotate3DXRadians(float radians) noexcept;
    void Rotate3DYRadians(float radians) noexcept;
    void Rotate3DZRadians(float radians) noexcept;
    void Rotate2DRadians(float radians) noexcept;

    void ConcatenateTransform(const Matrix4& other) noexcept;
    [[nodiscard]] Matrix4 GetTransformed(const Matrix4& other) const noexcept;

    [[nodiscard]] Vector2 TransformPosition(const Vector2& position) const noexcept;
    [[nodiscard]] Vector2 TransformDirection(const Vector2& direction) const noexcept;

    [[nodiscard]] Vector3 TransformPosition(const Vector3& position) const noexcept;
    [[nodiscard]] Vector3 TransformDirection(const Vector3& direction) const noexcept;

    [[nodiscard]] Vector4 TransformVector(const Vector4& homogeneousVector) const noexcept;
    [[nodiscard]] Vector3 TransformVector(const Vector3& homogeneousVector) const noexcept;
    [[nodiscard]] Vector2 TransformVector(const Vector2& homogeneousVector) const noexcept;

    [[nodiscard]] const float* GetAsFloatArray() const noexcept;
    [[nodiscard]] float* GetAsFloatArray() noexcept;

    [[nodiscard]] Vector3 GetTranslation() const noexcept;
    [[nodiscard]] Vector3 GetTranslation() noexcept;

    [[nodiscard]] Vector3 GetScale() const noexcept;
    [[nodiscard]] Vector3 GetScale() noexcept;

    [[nodiscard]] Matrix4 GetRotation() const noexcept;
    [[nodiscard]] Matrix4 GetRotation() noexcept;

    [[nodiscard]] Vector3 CalcEulerAngles() const noexcept;

    //SRTs must be calculated as T * R * S
    [[nodiscard]] Matrix4 operator*(const Matrix4& rhs) const noexcept;
    [[nodiscard]] Vector4 operator*(const Vector4& rhs) const noexcept;
    friend Vector4 operator*(const Vector4& lhs, const Matrix4& rhs) noexcept;
    [[nodiscard]] Vector3 operator*(const Vector3& rhs) const noexcept;
    friend Vector3 operator*(const Vector3& lhs, const Matrix4& rhs) noexcept;
    [[nodiscard]] Vector2 operator*(const Vector2& rhs) const noexcept;
    friend Vector2 operator*(const Vector2& lhs, const Matrix4& rhs) noexcept;
    Matrix4& operator*=(const Matrix4& rhs) noexcept;
    friend Matrix4 operator*(float lhs, const Matrix4& rhs) noexcept;
    [[nodiscard]] const float* operator*() const noexcept;
    [[nodiscard]] float* operator*() noexcept;

    [[nodiscard]] bool operator==(const Matrix4& rhs) const noexcept;
    [[nodiscard]] bool operator==(const Matrix4& rhs) noexcept;
    [[nodiscard]] bool operator!=(const Matrix4& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Matrix4& rhs) noexcept;
    [[nodiscard]] Matrix4 operator*(float scalar) const noexcept;
    Matrix4& operator*=(float scalar) noexcept;
    [[nodiscard]] Matrix4 operator+(const Matrix4& rhs) const noexcept;
    Matrix4& operator+=(const Matrix4& rhs) noexcept;
    [[nodiscard]] Matrix4 operator-(const Matrix4& rhs) const noexcept;
    Matrix4& operator-=(const Matrix4& rhs) noexcept;
    [[nodiscard]] Matrix4 operator-() const noexcept;
    [[nodiscard]] Matrix4 operator/(const Matrix4& rhs) noexcept;
    Matrix4& operator/=(const Matrix4& rhs) noexcept;

    friend std::ostream& operator<<(std::ostream& out_stream, const Matrix4& m) noexcept;
    friend std::istream& operator>>(std::istream& in_stream, Matrix4& m) noexcept;

    [[nodiscard]] Vector3 GetRight() const noexcept;
    [[nodiscard]] Vector3 GetUp() const noexcept;
    [[nodiscard]] Vector3 GetForward() const noexcept;

    [[nodiscard]] Vector2 GetRight2D() const noexcept;
    [[nodiscard]] Vector2 GetUp2D() const noexcept;
    [[nodiscard]] Vector2 GetForward2D() const noexcept;

    [[nodiscard]] Vector4 GetIBasis() const noexcept;
    [[nodiscard]] Vector4 GetIBasis() noexcept;

    [[nodiscard]] Vector4 GetJBasis() const noexcept;
    [[nodiscard]] Vector4 GetJBasis() noexcept;

    [[nodiscard]] Vector4 GetKBasis() const noexcept;
    [[nodiscard]] Vector4 GetKBasis() noexcept;

    [[nodiscard]] Vector4 GetTBasis() const noexcept;
    [[nodiscard]] Vector4 GetTBasis() noexcept;

    [[nodiscard]] Vector4 GetXComponents() const noexcept;
    [[nodiscard]] Vector4 GetXComponents() noexcept;

    [[nodiscard]] Vector4 GetYComponents() const noexcept;
    [[nodiscard]] Vector4 GetYComponents() noexcept;

    [[nodiscard]] Vector4 GetZComponents() const noexcept;
    [[nodiscard]] Vector4 GetZComponents() noexcept;

    [[nodiscard]] Vector4 GetWComponents() const noexcept;
    [[nodiscard]] Vector4 GetWComponents() noexcept;

    void SetIBasis(const Vector4& basis) noexcept;
    void SetJBasis(const Vector4& basis) noexcept;
    void SetKBasis(const Vector4& basis) noexcept;
    void SetTBasis(const Vector4& basis) noexcept;

    void SetXComponents(const Vector4& components) noexcept;
    void SetYComponents(const Vector4& components) noexcept;
    void SetZComponents(const Vector4& components) noexcept;
    void SetWComponents(const Vector4& components) noexcept;

protected:
    [[nodiscard]] const float& operator[](std::size_t index) const;
    [[nodiscard]] float& operator[](std::size_t index);

    void SetIndex(unsigned int index, float value) noexcept;
    [[nodiscard]] float GetIndex(unsigned int index) const noexcept;
    [[nodiscard]] float GetIndex(unsigned int index) noexcept;
    [[nodiscard]] float GetIndex(unsigned int col, unsigned int row) const noexcept;

    [[nodiscard]] static Matrix4 CreateTranslationMatrix(float x, float y, float z) noexcept;
    [[nodiscard]] static Matrix4 CreateScaleMatrix(float scale_x, float scale_y, float scale_z) noexcept;

    explicit Matrix4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33) noexcept;

private:
    //[00 01 02 03] [0   1  2  3]
    //[10 11 12 13] [4   5  6  7]
    //[20 21 22 23] [8   9 10 11]
    //[30 31 32 33] [12 13 14 15]

    std::array<float, 16> m_indicies{1.0f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 0.0f, 1.0f};

    friend class Quaternion;
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const Matrix4& m) noexcept;
}
