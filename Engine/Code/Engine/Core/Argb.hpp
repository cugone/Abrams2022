#pragma once

#include "Engine/Math/Vector3.hpp"
#include "Engine/Math/Vector4.hpp"

#include <ostream>
#include <string>
#include <vector>

class Rgba;

class Argb {
public:
    static const Argb White;
    static const Argb Black;
    static const Argb Red;
    static const Argb Pink;
    static const Argb Green;
    static const Argb ForestGreen;
    static const Argb Blue;
    static const Argb NavyBlue;
    static const Argb Cyan;
    static const Argb Yellow;
    static const Argb Magenta;
    static const Argb Orange;
    static const Argb Violet;
    static const Argb LightGrey;
    static const Argb LightGray;
    static const Argb Grey;
    static const Argb Gray;
    static const Argb DarkGrey;
    static const Argb DarkGray;
    static const Argb Olive;
    static const Argb SkyBlue;
    static const Argb Lime;
    static const Argb Teal;
    static const Argb Turquoise;
    static const Argb Taupe;
    static const Argb Umber;
    static const Argb BurntUmber;
    static const Argb Sienna;
    static const Argb RawSienna;
    static const Argb Periwinkle;
    static const Argb NormalZ;
    static const Argb NoAlpha;

    [[nodiscard]] static Argb Random() noexcept;
    [[nodiscard]] static Argb RandomGreyscale() noexcept;
    [[nodiscard]] static Argb RandomGrayscale() noexcept;
    [[nodiscard]] static Argb RandomWithAlpha() noexcept;
    [[nodiscard]] static Argb RandomLessThan(const Argb& color) noexcept;

    Argb() = default;
    Argb(const Argb& rhs) = default;
    Argb(Argb&& rhs) = default;
    Argb& operator=(const Argb& rhs) = default;
    Argb& operator=(Argb&& rhs) = default;
    ~Argb() = default;

    explicit Argb(const Rgba& rgba) noexcept;
    explicit Argb(Rgba&& rgba) noexcept;
    explicit Argb(const Vector4& fromFloats) noexcept;
    explicit Argb(std::string name) noexcept;
    explicit Argb(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) noexcept;
    explicit Argb(std::uint32_t rawValue);

    void SetAsBytes(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) noexcept;
    void SetAsFloats(float normalized_alpha, float normalized_red, float normalized_green, float normalized_blue) noexcept;
    void GetAsFloats(float& out_normalized_alpha, float& out_normalized_red, float& out_normalized_green, float& out_normalized_blue) const noexcept;
    [[nodiscard]] Vector4 GetArgbAsFloats() const noexcept;
    [[nodiscard]] Vector3 GetRgbAsFloats() const noexcept;
    void ScaleRGB(float scale) noexcept;
    void ScaleAlpha(float scale) noexcept;

    [[nodiscard]] uint32_t GetAsRawValue() const noexcept;
    void SetFromRawValue(uint32_t value) noexcept;
    void SetArgbFromRawValue(uint32_t value) noexcept;
    void SetRgbFromRawValue(uint32_t value) noexcept;
    void SetRgbFromFloats(const Vector3& value) noexcept;
    void SetArgbFromFloats(const Vector4& value) noexcept;
    [[nodiscard]] bool IsRgbEqual(const Argb& rhs) const noexcept;
    [[nodiscard]] bool operator==(const Argb& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Argb& rhs) const noexcept;
    [[nodiscard]] bool operator<(const Argb& rhs) const noexcept;

    Argb& operator+=(const Argb& rhs) noexcept;
    friend Argb operator+(Argb lhs, const Argb& rhs) noexcept;

    Argb& operator-=(const Argb& rhs) noexcept;
    friend Argb operator-(Argb lhs, const Argb& rhs) noexcept;

    Argb& operator++() noexcept;
    Argb operator++(int) noexcept;

    Argb& operator--() noexcept;
    Argb operator--(int) noexcept;

    unsigned char a = 255;
    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;

    friend std::ostream& operator<<(std::ostream& os, const Argb& rhs) noexcept;

protected:
private:
    void SetValueFromName(std::string name) noexcept;
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const Argb& clr) noexcept;
}
