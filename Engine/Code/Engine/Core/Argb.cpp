#include "Engine/Core/Argb.hpp"

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <iomanip>
#include <sstream>

const Argb Argb::White(255, 255, 255, 255);
const Argb Argb::Black(255, 0, 0, 0);
const Argb Argb::Red(255, 255, 0, 0);
const Argb Argb::Pink(255, 255, 192, 203);
const Argb Argb::Green(255, 0, 255, 0);
const Argb Argb::ForestGreen(255, 34, 139, 34);
const Argb Argb::Blue(255, 0, 0, 255);
const Argb Argb::NavyBlue(255, 0, 0, 128);
const Argb Argb::Cyan(255, 0, 255, 255);
const Argb Argb::Yellow(255, 255, 255, 0);
const Argb Argb::Magenta(255, 255, 0, 255);
const Argb Argb::Orange(255, 255, 165, 0);
const Argb Argb::Violet(255, 143, 0, 255);
const Argb Argb::Grey(255, 128, 128, 128);
const Argb Argb::Gray(255, 128, 128, 128);
const Argb Argb::LightGrey(255, 192, 192, 192);
const Argb Argb::LightGray(255, 192, 192, 192);
const Argb Argb::DarkGrey(255, 64, 64, 64);
const Argb Argb::DarkGray(255, 64, 64, 64);
const Argb Argb::Olive(255, 107, 142, 35);
const Argb Argb::SkyBlue(255, 45, 185, 255);
const Argb Argb::Lime(255, 227, 255, 0);
const Argb Argb::Teal(255, 0, 128, 128);
const Argb Argb::Turquoise(255, 64, 224, 208);
const Argb Argb::Taupe(uint32_t{0xFF483C32});
const Argb Argb::Umber(uint32_t{0xFF635147});
const Argb Argb::BurntUmber(uint32_t{0xFF8A3324});
const Argb Argb::Sienna(uint32_t{0xFF882D17});
const Argb Argb::RawSienna(uint32_t{0xFF965434});
const Argb Argb::Periwinkle(255, 204, 204, 255);
const Argb Argb::NormalZ(255, 128, 128, 255);
const Argb Argb::NoAlpha(0, 0, 0, 0);

namespace StringUtils {
std::string to_string(const Argb& clr) noexcept {
    std::ostringstream ss;
    ss << std::hex << clr;
    return ss.str();
}
} // namespace StringUtils

Argb Argb::Random() noexcept {
    return Argb(255, static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)));
}

Argb Argb::RandomGreyscale() noexcept {
    auto r = static_cast<unsigned char>(MathUtils::GetRandomLessThan(256));
    return Argb(255, r, r, r);
}

Argb Argb::RandomGrayscale() noexcept {
    return RandomGreyscale();
}

Argb Argb::RandomWithAlpha() noexcept {
    return Argb(static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)));
}

Argb Argb::RandomLessThan(const Argb& color) noexcept {
    return Argb(static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.a + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.r + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.g + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.b + 1)));
}

std::ostream& operator<<(std::ostream& os, const Argb& rhs) noexcept {
    if(os.flags() & std::ios_base::hex) {
        auto old_fmt = os.flags();
        auto old_fill = os.fill();
        auto old_w = os.width();
        os << '#';
        os << std::setw(8);
        os << std::right;
        os << std::setfill('0');
        os << rhs.GetAsRawValue();
        os.flags(old_fmt);
        os.fill(old_fill);
        os.width(old_w);
        return os;
    }
    os << static_cast<int>(rhs.a) << ',' << static_cast<int>(rhs.r) << ',' << static_cast<int>(rhs.g) << ',' << static_cast<int>(rhs.b);
    return os;
}

Argb::Argb(const Rgba& rgba) noexcept
: a{rgba.a}
, r{rgba.r}
, g{rgba.g}
, b{rgba.b} {
    /* DO NOTHING */
}

Argb::Argb(Rgba&& rgba) noexcept
: a(rgba.a)
, r(rgba.r)
, g(rgba.g)
, b(rgba.b) {
    rgba.SetFromRawValue(std::uint32_t{0u});
}

Argb::Argb(const Vector4& fromFloats) noexcept {
    SetArgbFromFloats(fromFloats);
}

Argb::Argb(std::string name) noexcept {
    name = StringUtils::ToUpperCase(name);

    if(std::size_t hash_loc = name.find_first_of('#'); hash_loc != std::string::npos) {
        name.replace(hash_loc, 1, "0X");
        std::size_t char_count = 0;
        unsigned long value_int = std::stoul(name, &char_count, 16);
        if(char_count == 10) { //0xAARRGGBB
            SetArgbFromRawValue(value_int);
        } else if(char_count == 8) { //0xRRGGBB
            SetRgbFromRawValue(value_int);
            a = 255;
        } else {
            /* DO NOTHING */
        }
    } else {
        const auto v = StringUtils::Split(name);
        const auto v_s = v.size();
        if(v_s > 1) {
            if(!(v_s < 3)) {
                if(v_s > 3) {
                    a = static_cast<unsigned char>(std::stoi(v[0].data()));
                    r = static_cast<unsigned char>(std::stoi(v[1].data()));
                    g = static_cast<unsigned char>(std::stoi(v[2].data()));
                    b = static_cast<unsigned char>(std::stoi(v[3].data()));
                } else {
                    r = static_cast<unsigned char>(std::stoi(v[0].data()));
                    g = static_cast<unsigned char>(std::stoi(v[1].data()));
                    b = static_cast<unsigned char>(std::stoi(v[2].data()));
                    a = 255;
                }
            }
        } else {
            SetValueFromName(v_s ? v[0] : "");
        }
    }
}

Argb::Argb(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) noexcept
: a(alpha)
, r(red)
, g(green)
, b(blue) {
    /* DO NOTHING */
}

Argb::Argb(std::uint32_t rawValue) {
    SetFromRawValue(rawValue);
}

void Argb::SetAsBytes(unsigned char alpha, unsigned char red, unsigned char green, unsigned char blue) noexcept {
    a = alpha;
    r = red;
    g = green;
    b = blue;
}

void Argb::SetAsFloats(float normalized_alpha, float normalized_red, float normalized_green, float normalized_blue) noexcept {
    a = static_cast<unsigned char>(normalized_alpha * 255.0f);
    r = static_cast<unsigned char>(normalized_red * 255.0f);
    g = static_cast<unsigned char>(normalized_green * 255.0f);
    b = static_cast<unsigned char>(normalized_blue * 255.0f);
}

void Argb::GetAsFloats(float& out_normalized_alpha, float& out_normalized_red, float& out_normalized_green, float& out_normalized_blue) const noexcept {
    out_normalized_alpha = a / 255.0f;
    out_normalized_red = r / 255.0f;
    out_normalized_green = g / 255.0f;
    out_normalized_blue = b / 255.0f;
}

Vector4 Argb::GetArgbAsFloats() const noexcept {
    return Vector4{a / 255.0f, r / 255.0f, g / 255.0f, b / 255.0f};
}

Vector3 Argb::GetRgbAsFloats() const noexcept {
    return Vector3{r / 255.0f, g / 255.0f, b / 255.0f};
}

void Argb::ScaleRGB(float scale) noexcept {
    const auto scaled_red = static_cast<float>(r) * scale;
    const auto scaled_green = static_cast<float>(g) * scale;
    const auto scaled_blue = static_cast<float>(b) * scale;
    r = static_cast<unsigned char>(std::clamp(scaled_red, 0.0f, 255.0f));
    g = static_cast<unsigned char>(std::clamp(scaled_green, 0.0f, 255.0f));
    b = static_cast<unsigned char>(std::clamp(scaled_blue, 0.0f, 255.0f));
}

void Argb::ScaleAlpha(float scale) noexcept {
    const auto scaled_alpha = static_cast<float>(a) * scale;
    a = static_cast<unsigned char>(std::clamp(scaled_alpha, 0.0f, 255.0f));
}

uint32_t Argb::GetAsRawValue() const noexcept {
    return static_cast<uint32_t>(((static_cast<uint32_t>(a) << 24) & 0xFF000000u)
                                 | ((static_cast<uint32_t>(r) << 16) & 0x00FF0000u)
                                 | ((static_cast<uint32_t>(g) << 8) & 0x0000FF00u)
                                 | ((static_cast<uint32_t>(b) << 0) & 0x000000FFu));
}

void Argb::SetFromRawValue(uint32_t value) noexcept {
    SetArgbFromRawValue(value);
}

void Argb::SetArgbFromRawValue(uint32_t value) noexcept {
    a = static_cast<uint8_t>((value & 0xFF000000u) >> 24);
    r = static_cast<uint8_t>((value & 0x00FF0000u) >> 16);
    g = static_cast<uint8_t>((value & 0x0000FF00u) >> 8);
    b = static_cast<uint8_t>((value & 0x000000FFu) >> 0);
}

void Argb::SetRgbFromRawValue(uint32_t value) noexcept {
    r = static_cast<uint8_t>((value & 0x00FF0000u) >> 16);
    g = static_cast<uint8_t>((value & 0x0000FF00u) >> 8);
    b = static_cast<uint8_t>((value & 0x000000FFu) >> 0);
}

void Argb::SetRgbFromFloats(const Vector3& value) noexcept {
    r = static_cast<unsigned char>(value.x * 255.0f);
    g = static_cast<unsigned char>(value.y * 255.0f);
    b = static_cast<unsigned char>(value.z * 255.0f);
}

void Argb::SetArgbFromFloats(const Vector4& value) noexcept {
    a = static_cast<unsigned char>(value.x * 255.0f);
    r = static_cast<unsigned char>(value.y * 255.0f);
    g = static_cast<unsigned char>(value.z * 255.0f);
    b = static_cast<unsigned char>(value.w * 255.0f);
}

void Argb::SetValueFromName(std::string name) noexcept {
    name = StringUtils::ToUpperCase(name);
    if(name == "WHITE") {
        SetFromRawValue(Argb::White.GetAsRawValue());
    } else if(name == "BLACK") {
        SetFromRawValue(Argb::Black.GetAsRawValue());
    } else if(name == "RED") {
        SetFromRawValue(Argb::Red.GetAsRawValue());
    } else if(name == "GREEN") {
        SetFromRawValue(Argb::Green.GetAsRawValue());
    } else if(name == "FORESTGREEN") {
        SetFromRawValue(Argb::ForestGreen.GetAsRawValue());
    } else if(name == "NAVYBLUE") {
        SetFromRawValue(Argb::NavyBlue.GetAsRawValue());
    } else if(name == "CYAN") {
        SetFromRawValue(Argb::Cyan.GetAsRawValue());
    } else if(name == "YELLOW") {
        SetFromRawValue(Argb::Yellow.GetAsRawValue());
    } else if(name == "MAGENTA") {
        SetFromRawValue(Argb::Magenta.GetAsRawValue());
    } else if(name == "ORANGE") {
        SetFromRawValue(Argb::Orange.GetAsRawValue());
    } else if(name == "GREY") {
        SetFromRawValue(Argb::Grey.GetAsRawValue());
    } else if(name == "GRAY") {
        SetFromRawValue(Argb::Gray.GetAsRawValue());
    } else if(name == "LIGHTGREY") {
        SetFromRawValue(Argb::LightGrey.GetAsRawValue());
    } else if(name == "LIGHTGRAY") {
        SetFromRawValue(Argb::LightGray.GetAsRawValue());
    } else if(name == "DARKGREY") {
        SetFromRawValue(Argb::DarkGrey.GetAsRawValue());
    } else if(name == "DARKGRAY") {
        SetFromRawValue(Argb::DarkGray.GetAsRawValue());
    } else if(name == "OLIVE") {
        SetFromRawValue(Argb::Olive.GetAsRawValue());
    } else if(name == "SKYBLUE") {
        SetFromRawValue(Argb::SkyBlue.GetAsRawValue());
    } else if(name == "LIME") {
        SetFromRawValue(Argb::Lime.GetAsRawValue());
    } else if(name == "TEAL") {
        SetFromRawValue(Argb::Teal.GetAsRawValue());
    } else if(name == "TURQUOISE") {
        SetFromRawValue(Argb::Turquoise.GetAsRawValue());
    } else if(name == "PERIWINKLE") {
        SetFromRawValue(Argb::Periwinkle.GetAsRawValue());
    } else if(name == "NORMALZ") {
        SetFromRawValue(Argb::NormalZ.GetAsRawValue());
    } else if(name == "NOALPHA") {
        SetFromRawValue(Argb::NoAlpha.GetAsRawValue());
    } else if(name == "PINK") {
        SetFromRawValue(Argb::Pink.GetAsRawValue());
    } else if(name == "BLUE") {
        SetFromRawValue(Argb::Blue.GetAsRawValue());
    } else if(name == "VIOLET") {
        SetFromRawValue(Argb::Violet.GetAsRawValue());
    } else if(name == "TAUPE") {
        SetFromRawValue(Argb::Taupe.GetAsRawValue());
    } else if(name == "UMBER") {
        SetFromRawValue(Argb::Umber.GetAsRawValue());
    } else if(name == "BURNTUMBER") {
        SetFromRawValue(Argb::BurntUmber.GetAsRawValue());
    } else if(name == "SIENNA") {
        SetFromRawValue(Argb::Sienna.GetAsRawValue());
    } else if(name == "RAWSIENNA") {
        SetFromRawValue(Argb::RawSienna.GetAsRawValue());
    }
}

bool Argb::IsRgbEqual(const Argb& rhs) const noexcept {
    return r == rhs.r && g == rhs.g && b == rhs.b;
}

bool Argb::operator<(const Argb& rhs) const noexcept {
    return GetAsRawValue() < rhs.GetAsRawValue();
}

Argb operator+(Argb lhs, const Argb& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

Argb& Argb::operator+=(const Argb& rhs) noexcept {
    int a_int = a + rhs.a;
    int r_int = r + rhs.r;
    int g_int = g + rhs.g;
    int b_int = b + rhs.b;
    a = static_cast<unsigned char>(std::clamp(a_int, 0, 255));
    r = static_cast<unsigned char>(std::clamp(r_int, 0, 255));
    g = static_cast<unsigned char>(std::clamp(g_int, 0, 255));
    b = static_cast<unsigned char>(std::clamp(b_int, 0, 255));
    return *this;
}

Argb operator-(Argb lhs, const Argb& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

Argb& Argb::operator-=(const Argb& rhs) noexcept {
    int a_int = a - rhs.a;
    int r_int = r - rhs.r;
    int g_int = g - rhs.g;
    int b_int = b - rhs.b;
    a = static_cast<unsigned char>(std::clamp(a_int, 0, 255));
    r = static_cast<unsigned char>(std::clamp(r_int, 0, 255));
    g = static_cast<unsigned char>(std::clamp(g_int, 0, 255));
    b = static_cast<unsigned char>(std::clamp(b_int, 0, 255));
    return *this;
}

Argb& Argb::operator++() noexcept {
    const auto raw = GetAsRawValue();
    SetFromRawValue(raw + 1);
    return *this;
}
Argb Argb::operator++(int) noexcept {
    Argb temp(*this);
    operator++();
    return temp;
}

Argb& Argb::operator--() noexcept {
    const auto raw = GetAsRawValue();
    SetFromRawValue(raw - 1);
    return *this;
}
Argb Argb::operator--(int) noexcept {
    Argb temp(*this);
    operator--();
    return temp;
}

bool Argb::operator!=(const Argb& rhs) const noexcept {
    return !(*this == rhs);
}

bool Argb::operator==(const Argb& rhs) const noexcept {
    return a == rhs.a && r == rhs.r && g == rhs.g && b == rhs.b;
}
