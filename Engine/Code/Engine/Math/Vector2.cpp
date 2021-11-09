#include "Engine/Math/Vector2.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <cmath>
#include <sstream>

const Vector2 Vector2::Zero(0.0f, 0.0f);
const Vector2 Vector2::X_Axis(1.0f, 0.0f);
const Vector2 Vector2::Y_Axis(0.0f, 1.0f);
const Vector2 Vector2::One(1.0f, 1.0);
const Vector2 Vector2::XY_Axis(1.0f, 1.0);
const Vector2 Vector2::YX_Axis(1.0f, 1.0);

Vector2::Vector2(float initialX, float initialY) noexcept
: x(initialX)
, y(initialY) {
    /* DO NOTHING */
}

Vector2::Vector2(const Vector3& rhs) noexcept
: x(rhs.x)
, y(rhs.y) {
    /* DO NOTHING */
}

Vector2::Vector2(const std::string& value) noexcept
: x(0.0f)
, y(0.0f) {
    if(!value.empty()) {
        if(value.front() == '[') {
            if(value.back() == ']') {
                const auto contents_str = std::string{std::begin(value) + 1, std::end(value) - 1};
                const auto&& values = StringUtils::Split(contents_str);
                x = std::stof(values[0]);
                y = std::stof(values[1]);
            }
        }
    }
}

Vector2::Vector2(const IntVector2& intvec2) noexcept
: x(static_cast<float>(intvec2.x))
, y(static_cast<float>(intvec2.y)) {
    /* DO NOTHING */
}

Vector2 Vector2::operator+(const Vector2& rhs) const noexcept {
    return Vector2(x + rhs.x, y + rhs.y);
}

Vector2& Vector2::operator+=(const Vector2& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

Vector2 Vector2::operator-(const Vector2& rhs) const noexcept {
    return Vector2(x - rhs.x, y - rhs.y);
}

Vector2& Vector2::operator-=(const Vector2& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

Vector2 Vector2::operator-() const noexcept {
    return Vector2(-x, -y);
}

Vector2 Vector2::operator*(const Vector2& rhs) const noexcept {
    return Vector2(x * rhs.x, y * rhs.y);
}

Vector2 operator*(float lhs, const Vector2& rhs) noexcept {
    return Vector2(lhs * rhs.x, lhs * rhs.y);
}

Vector2 Vector2::operator*(float scalar) const noexcept {
    return Vector2(x * scalar, y * scalar);
}

Vector2& Vector2::operator*=(float scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2& Vector2::operator*=(const Vector2& rhs) noexcept {
    x *= rhs.x;
    y *= rhs.y;
    return *this;
}

Vector2 Vector2::operator/(float scalar) const noexcept {
    return Vector2(x / scalar, y / scalar);
}

Vector2 Vector2::operator/=(float scalar) noexcept {
    x /= scalar;
    y /= scalar;
    return *this;
}

Vector2 Vector2::operator/(const Vector2& rhs) const noexcept {
    return Vector2(x / rhs.x, y / rhs.y);
}

Vector2 Vector2::operator/=(const Vector2& rhs) noexcept {
    x /= rhs.x;
    y /= rhs.y;
    return *this;
}

bool Vector2::operator==(const Vector2& rhs) const noexcept {
    return x == rhs.x && y == rhs.y;
}

bool Vector2::operator!=(const Vector2& rhs) const noexcept {
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out_stream, const Vector2& v) noexcept {
    out_stream << '[' << v.x << ',' << v.y << ']';
    return out_stream;
}

std::istream& operator>>(std::istream& in_stream, Vector2& v) noexcept {
    auto x = 0.0f;
    auto y = 0.0f;

    in_stream.ignore(); //[
    in_stream >> x;
    in_stream.ignore(); //,
    in_stream >> y;
    in_stream.ignore(); //]

    v.x = x;
    v.y = y;

    return in_stream;
}

Vector2 Vector2::CreateFromPolarCoordinatesDegrees(float length, float directionDegrees) {
    return CreateFromPolarCoordinatesRadians(length, MathUtils::ConvertDegreesToRadians(directionDegrees));
}

Vector2 Vector2::CreateFromPolarCoordinatesRadians(float length, float directionRadians) {
    return Vector2{length * std::cos(directionRadians), length * std::sin(directionRadians)};
}

std::tuple<float, float> Vector2::GetXY() const noexcept {
    return std::make_tuple(x, y);
}

const float* Vector2::GetAsFloatArray() const noexcept {
    return &x;
}

float* Vector2::GetAsFloatArray() noexcept {
    return &x;
}

float Vector2::CalcHeadingRadians() const noexcept {
    return std::atan2(y, x);
}

float Vector2::CalcHeadingDegrees() const noexcept {
    return MathUtils::ConvertRadiansToDegrees(CalcHeadingRadians());
}

float Vector2::CalcLength() const noexcept {
    return std::sqrt(CalcLengthSquared());
}

float Vector2::CalcLengthSquared() const noexcept {
    return x * x + y * y;
}

void Vector2::SetHeadingDegrees(float headingDegrees) noexcept {
    SetHeadingRadians(MathUtils::ConvertDegreesToRadians(headingDegrees));
}

void Vector2::SetHeadingRadians(float headingRadians) noexcept {
    const auto R = CalcLength();
    const auto theta = headingRadians;
    x = R * std::cos(theta);
    y = R * std::sin(theta);
}

void Vector2::SetUnitLengthAndHeadingDegrees(float headingDegrees) noexcept {
    SetUnitLengthAndHeadingRadians(MathUtils::ConvertDegreesToRadians(headingDegrees));
}

void Vector2::SetUnitLengthAndHeadingRadians(float headingRadians) noexcept {
    Normalize();
    SetHeadingRadians(headingRadians);
}

float Vector2::SetLength(float length) noexcept {
    const auto R = CalcLength();
    const auto theta = CalcHeadingRadians();
    x = length * std::cos(theta);
    y = length * std::sin(theta);
    return R;
}

void Vector2::SetLengthAndHeadingDegrees(float headingDegrees, float length) noexcept {
    SetLengthAndHeadingRadians(MathUtils::ConvertDegreesToRadians(headingDegrees), length);
}

void Vector2::SetLengthAndHeadingRadians(float headingRadians, float length) noexcept {
    SetLength(length);
    SetHeadingRadians(headingRadians);
}

void Vector2::RotateRadians(float radians) noexcept {
    const auto R = CalcLength();
    const auto old_angle = std::atan2(y, x);
    const auto new_angle = old_angle + radians;

    x = R * std::cos(new_angle);
    y = R * std::sin(new_angle);
}

void Vector2::RotateDegrees(float degrees) noexcept {
    RotateRadians(MathUtils::ConvertDegreesToRadians(degrees));
}

float Vector2::Normalize() noexcept {
    const auto length = CalcLength();
    if(length > 0.0f) {
        const auto inv_length = 1.0f / length;
        x *= inv_length;
        y *= inv_length;
        return length;
    }
    return 0.0f;
}

Vector2 Vector2::GetNormalize() const noexcept {
    const auto length = CalcLength();
    if(length > 0.0f) {
        const auto inv_length = 1.0f / length;
        return Vector2(x * inv_length, y * inv_length);
    }
    return Vector2::Zero;
}

Vector2 Vector2::GetLeftHandNormal() const noexcept {
    Vector2 result = *this;
    result.Rotate90Degrees();
    return result;
}

Vector2 Vector2::GetRightHandNormal() const noexcept {
    Vector2 result = *this;
    result.RotateNegative90Degrees();
    return result;
}

void Vector2::Rotate90Degrees() noexcept {
    SetXY(-y, x);
}

void Vector2::RotateNegative90Degrees() noexcept {
    SetXY(y, -x);
}

void Vector2::SetXY(float newX, float newY) noexcept {
    x = newX;
    y = newY;
}

void swap(Vector2& a, Vector2& b) noexcept {
    std::swap(a.x, b.x);
    std::swap(a.y, b.y);
}

std::string StringUtils::to_string(const Vector2& v) noexcept {
    std::ostringstream ss;
    ss << '[' << v.x << ',' << v.y << ']';
    return ss.str();
}
