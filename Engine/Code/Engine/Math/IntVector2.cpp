#include "Engine/Math/IntVector2.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVector3.hpp"
#include "Engine/Math/Vector2.hpp"

#include <cmath>
#include <sstream>

const IntVector2 IntVector2::Zero(0, 0);
const IntVector2 IntVector2::One(1, 1);
const IntVector2 IntVector2::X_Axis(1, 0);
const IntVector2 IntVector2::Y_Axis(0, 1);
const IntVector2 IntVector2::XY_Axis(1, 1);
const IntVector2 IntVector2::YX_Axis(1, 1);

IntVector2::IntVector2(int initialX, int initialY) noexcept
: x(initialX)
, y(initialY) {
    /* DO NOTHING */
}

IntVector2::IntVector2(const Vector2& v2) noexcept
: x(static_cast<int>(std::floor(v2.x)))
, y(static_cast<int>(std::floor(v2.y))) {
    /* DO NOTHING */
}

IntVector2::IntVector2(const IntVector3& iv3) noexcept
: x(iv3.x)
, y(iv3.y) {
    /* DO NOTHING */
}

IntVector2::IntVector2(const std::string& value) noexcept
: x(0)
, y(0) {
    if(!value.empty()) {
        if(value.front() == '[') {
            if(value.back() == ']') {
                const auto contents_str = std::string{std::begin(value) + 1, std::end(value) - 1};
                const auto&& values = StringUtils::Split(contents_str);
                x = std::stoi(values[0]);
                y = std::stoi(values[1]);
            }
        }
    }
}

IntVector2 IntVector2::operator-() const noexcept {
    return IntVector2(-x, -y);
}

IntVector2 IntVector2::operator+(const IntVector2& rhs) const noexcept {
    return IntVector2(x + rhs.x, y + rhs.y);
}

IntVector2& IntVector2::operator+=(const IntVector2& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

IntVector2 IntVector2::operator-(const IntVector2& rhs) const noexcept {
    return IntVector2(x - rhs.x, y - rhs.y);
}

IntVector2& IntVector2::operator-=(const IntVector2& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

std::ostream& operator<<(std::ostream& out_stream, const IntVector2& v) noexcept {
    out_stream << '[' << v.x << ',' << v.y << ']';
    return out_stream;
}

std::istream& operator>>(std::istream& in_stream, IntVector2& v) noexcept {
    auto x = 0;
    auto y = 0;

    in_stream.ignore(); //[
    in_stream >> x;
    in_stream.ignore(); //,
    in_stream >> y;
    in_stream.ignore(); //]

    v.x = x;
    v.y = y;

    return in_stream;
}
IntVector2 operator*(int lhs, const IntVector2& rhs) noexcept {
    return IntVector2(lhs * rhs.x, lhs * rhs.y);
}

IntVector2 IntVector2::operator*(const IntVector2& rhs) const noexcept {
    return IntVector2(x * rhs.x, y * rhs.y);
}

IntVector2& IntVector2::operator*=(const IntVector2& rhs) noexcept {
    x *= rhs.x;
    y *= rhs.y;
    return *this;
}

IntVector2 IntVector2::operator*(int scalar) const noexcept {
    return IntVector2(x * scalar, y * scalar);
}

IntVector2& IntVector2::operator*=(int scalar) noexcept {
    x *= scalar;
    y *= scalar;
    return *this;
}

IntVector2 IntVector2::operator*(float scalar) const noexcept {
    const auto nx = static_cast<int>(std::floor(static_cast<float>(x) * scalar));
    const auto ny = static_cast<int>(std::floor(static_cast<float>(y) * scalar));
    return IntVector2(nx, ny);
}

IntVector2& IntVector2::operator*=(float scalar) noexcept {
    x = static_cast<int>(std::floor(static_cast<float>(x) * scalar));
    y = static_cast<int>(std::floor(static_cast<float>(y) * scalar));
    return *this;
}

IntVector2 IntVector2::operator/(const IntVector2& rhs) const noexcept {
    return IntVector2(x / rhs.x, y / rhs.y);
}

IntVector2& IntVector2::operator/=(const IntVector2& rhs) noexcept {
    x /= rhs.x;
    y /= rhs.y;
    return *this;
}

IntVector2 IntVector2::operator/(int scalar) const noexcept {
    return IntVector2(x / scalar, y / scalar);
}

IntVector2& IntVector2::operator/=(int scalar) noexcept {
    x /= scalar;
    y /= scalar;
    return *this;
}

IntVector2 IntVector2::operator/(float scalar) const noexcept {
    const auto nx = static_cast<int>(std::floor(static_cast<float>(x) / scalar));
    const auto ny = static_cast<int>(std::floor(static_cast<float>(y) / scalar));
    return IntVector2(nx, ny);
}

IntVector2& IntVector2::operator/=(float scalar) noexcept {
    x = static_cast<int>(std::floor(static_cast<float>(x) / scalar));
    y = static_cast<int>(std::floor(static_cast<float>(y) / scalar));
    return *this;
}

void IntVector2::SetXY(int newX, int newY) noexcept {
    x = newX;
    y = newY;
}

std::pair<int, int> IntVector2::GetXY() const noexcept {
    return std::make_pair(x, y);
}

bool IntVector2::operator!=(const IntVector2& rhs) const noexcept {
    return !(*this == rhs);
}

bool IntVector2::operator==(const IntVector2& rhs) const noexcept {
    return x == rhs.x && y == rhs.y;
}

bool IntVector2::operator<(const IntVector2& rhs) const noexcept {
    if(x < rhs.x)
        return true;
    if(rhs.x < x)
        return false;
    if(y < rhs.y)
        return true;
    return false;
}

bool IntVector2::operator>=(const IntVector2& rhs) const noexcept {
    return !(*this < rhs);
}

bool IntVector2::operator>(const IntVector2& rhs) const noexcept {
    return rhs < *this;
}

bool IntVector2::operator<=(const IntVector2& rhs) const noexcept {
    return !(*this > rhs);
}

std::string StringUtils::to_string(const IntVector2& v) noexcept {
    std::ostringstream ss;
    ss << '[' << v.x << ',' << v.y << ']';
    return ss.str();
}
