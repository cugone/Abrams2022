#pragma once

#include <initializer_list>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

class Argb;

class Rgba {
public:
    static const Rgba White;
    static const Rgba Black;
    static const Rgba Red;
    static const Rgba Pink;
    static const Rgba Green;
    static const Rgba ForestGreen;
    static const Rgba Blue;
    static const Rgba NavyBlue;
    static const Rgba MidnightBlue;
    static const Rgba Cyan;
    static const Rgba Yellow;
    static const Rgba Magenta;
    static const Rgba Orange;
    static const Rgba Violet;
    static const Rgba LightGrey;
    static const Rgba LightGray;
    static const Rgba Grey;
    static const Rgba Gray;
    static const Rgba DarkGrey;
    static const Rgba DarkGray;
    static const Rgba Olive;
    static const Rgba SkyBlue;
    static const Rgba Lime;
    static const Rgba Teal;
    static const Rgba Turquoise;
    static const Rgba Taupe;
    static const Rgba Umber;
    static const Rgba BurntUmber;
    static const Rgba Sienna;
    static const Rgba RawSienna;
    static const Rgba Periwinkle;
    static const Rgba NormalZ;
    static const Rgba NoAlpha;

    [[nodiscard]] static Rgba Random() noexcept;
    [[nodiscard]] static Rgba RandomGreyscale() noexcept;
    [[nodiscard]] static Rgba RandomGrayscale() noexcept;
    [[nodiscard]] static Rgba RandomWithAlpha() noexcept;
    [[nodiscard]] static Rgba RandomLessThan(const Rgba& color) noexcept;

    Rgba() = default;
    Rgba(const Rgba& rhs) = default;
    Rgba(Rgba&& rhs) = default;
    Rgba& operator=(const Rgba& rhs) = default;
    Rgba& operator=(Rgba&& rhs) = default;
    ~Rgba() = default;

    explicit Rgba(const Argb& argb) noexcept;
    explicit Rgba(Argb&& argb) noexcept;
    explicit Rgba(std::initializer_list<float> fromFloats) noexcept;
    explicit Rgba(std::initializer_list<unsigned char> fromUChar) noexcept;
    explicit Rgba(std::string name) noexcept;
    explicit Rgba(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 0xFF) noexcept;
    explicit Rgba(uint32_t rawValue);

    void SetAsBytes(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) noexcept;
    void SetAsFloats(float normalized_red, float normalized_green, float normalized_blue, float normalized_alpha) noexcept;
    [[nodiscard]] std::tuple<float, float, float, float> GetAsFloats() const noexcept;
    void ScaleRGB(float scale) noexcept;
    void ScaleAlpha(float scale) noexcept;
    void Invert() noexcept;
    void InvertRGB() noexcept;
    void InvertAlpha() noexcept;

    [[nodiscard]] uint32_t GetAsRawValue() const noexcept;
    void SetFromRawValue(uint32_t value) noexcept;
    void SetRGBAFromRawValue(uint32_t value) noexcept;
    void SetRGBFromRawValue(uint32_t value) noexcept;
    void SetFromFloats(std::initializer_list<float> ilist) noexcept;
    [[nodiscard]] bool IsRgbEqual(const Rgba& rhs) const noexcept;
    [[nodiscard]] bool operator==(const Rgba& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const Rgba& rhs) const noexcept;
    [[nodiscard]] bool operator<(const Rgba& rhs) const noexcept;

    Rgba& operator+=(const Rgba& rhs) noexcept;
    friend Rgba operator+(Rgba lhs, const Rgba& rhs) noexcept;

    Rgba& operator-=(const Rgba& rhs) noexcept;
    friend Rgba operator-(Rgba lhs, const Rgba& rhs) noexcept;

    Rgba& operator++() noexcept;
    Rgba operator++(int) noexcept;

    Rgba& operator--() noexcept;
    Rgba operator--(int) noexcept;

    unsigned char r = 255;
    unsigned char g = 255;
    unsigned char b = 255;
    unsigned char a = 255;

    friend std::ostream& operator<<(std::ostream& os, const Rgba& rhs) noexcept;

protected:
private:
    void SetValueFromName(std::string name) noexcept;
};

namespace StringUtils {
[[nodiscard]] std::string to_string(const Rgba& clr) noexcept;
}
