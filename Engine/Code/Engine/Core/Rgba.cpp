#include "Engine/Core/Rgba.hpp"

#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <iomanip>
#include <sstream>
#include <utility>

const Rgba Rgba::White(255, 255, 255, 255);
const Rgba Rgba::Black(0, 0, 0, 255);
const Rgba Rgba::Red(255, 0, 0, 255);
const Rgba Rgba::Pink(255, 192, 203, 255);
const Rgba Rgba::Green(0, 255, 0, 255);
const Rgba Rgba::ForestGreen(34, 139, 34, 255);
const Rgba Rgba::Blue(0, 0, 255, 255);
const Rgba Rgba::NavyBlue(0, 0, 128, 255);
const Rgba Rgba::MidnightBlue(25, 25, 112, 255);
const Rgba Rgba::Cyan(0, 255, 255, 255);
const Rgba Rgba::Yellow(255, 255, 0, 255);
const Rgba Rgba::Magenta(255, 0, 255, 255);
const Rgba Rgba::Orange(255, 165, 0, 255);
const Rgba Rgba::Violet(143, 0, 255, 255);
const Rgba Rgba::Grey(128, 128, 128, 255);
const Rgba Rgba::Gray(128, 128, 128, 255);
const Rgba Rgba::LightGrey(192, 192, 192, 255);
const Rgba Rgba::LightGray(192, 192, 192, 255);
const Rgba Rgba::DarkGrey(64, 64, 64, 255);
const Rgba Rgba::DarkGray(64, 64, 64, 255);
const Rgba Rgba::Olive(107, 142, 35, 255);
const Rgba Rgba::SkyBlue(45, 185, 255, 255);
const Rgba Rgba::Lime(227, 255, 0, 255);
const Rgba Rgba::Teal(0, 128, 128, 255);
const Rgba Rgba::Turquoise(64, 224, 208, 255);
const Rgba Rgba::Taupe(uint32_t{0x483C32FF});
const Rgba Rgba::Umber(uint32_t{0x635147FF});
const Rgba Rgba::BurntUmber(uint32_t{0x8A3324FF});
const Rgba Rgba::Sienna(uint32_t{0x882D17FF});
const Rgba Rgba::RawSienna(uint32_t{0x965434FF});
const Rgba Rgba::Periwinkle(204, 204, 255, 255);
const Rgba Rgba::NormalZ(128, 128, 255, 255);
const Rgba Rgba::NoAlpha(0, 0, 0, 0);

namespace StringUtils {
std::string to_string(const Rgba& clr) noexcept {
    std::ostringstream ss;
    ss << std::hex << clr;
    return ss.str();
}
} // namespace StringUtils

Rgba Rgba::Random() noexcept {
    return Rgba(static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), 255);
}

Rgba Rgba::RandomGreyscale() noexcept {
    auto r = static_cast<unsigned char>(MathUtils::GetRandomLessThan(256));
    return Rgba(r, r, r, 255);
}

Rgba Rgba::RandomGrayscale() noexcept {
    return RandomGreyscale();
}

Rgba Rgba::RandomWithAlpha() noexcept {
    return Rgba(static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(256)));
}

Rgba Rgba::RandomLessThan(const Rgba& color) noexcept {
    return Rgba(static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.r + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.g + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.b + 1)), static_cast<unsigned char>(MathUtils::GetRandomLessThan(color.a + 1)));
}

std::ostream& operator<<(std::ostream& os, const Rgba& rhs) noexcept {
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
    os << static_cast<int>(rhs.r) << ',' << static_cast<int>(rhs.g) << ',' << static_cast<int>(rhs.b) << ',' << static_cast<int>(rhs.a);
    return os;
}

Rgba::Rgba(std::initializer_list<float> fromFloats) noexcept {
    SetFromFloats(fromFloats);
}

Rgba::Rgba(std::initializer_list<unsigned char> ilist) noexcept {
    const auto length = ilist.size();
    constexpr auto convertToUChar = [](auto&& v) -> unsigned char {
        const auto clamped_v = std::clamp(std::forward<decltype(v)>(std::remove_pointer_t<decltype(v)>(std::decay_t<decltype(v)>(v))), uint8_t{0u}, uint8_t{255u});
        return static_cast<unsigned char>(clamped_v);
    };
    switch(length) {
    case 1: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*std::begin(ilist));
        b = convertToUChar(*std::begin(ilist));
        a = convertToUChar(*std::begin(ilist));
        break;
    }
    case 2: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        break;
    }
    case 3: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        b = convertToUChar(*(std::next(std::next(std::begin(ilist)))));
        break;
    }
    default: {
        /* DO NOTHING */
        break;
    }
    }
    if(length >= 4) {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        b = convertToUChar(*(std::next(std::next(std::begin(ilist)))));
        a = convertToUChar(*(std::next(std::next(std::next(std::begin(ilist))))));
    }
}

Rgba::Rgba(std::string name) noexcept {
    name = StringUtils::ToUpperCase(name);

    if(std::size_t hash_loc = name.find_first_of('#'); hash_loc != std::string::npos) {
        name.replace(hash_loc, 1, "0X");
        std::size_t char_count = 0;
        unsigned long value_int = std::stoul(name, &char_count, 16);
        if(char_count == 10) { //0xRRGGBBAA
            SetRGBAFromRawValue(value_int);
        } else if(char_count == 8) { //0xRRGGBB
            SetRGBFromRawValue(value_int);
            a = 255;
        } else {
            /* DO NOTHING */
        }
    } else {
        const auto v = StringUtils::Split(name);
        const auto v_s = v.size();
        if(v_s > 1) {
            if(!(v_s < 3)) {
                r = static_cast<unsigned char>(std::stoi(v[0].data()));
                g = static_cast<unsigned char>(std::stoi(v[1].data()));
                b = static_cast<unsigned char>(std::stoi(v[2].data()));
                if(v_s > 3) {
                    a = static_cast<unsigned char>(std::stoi(v[3].data()));
                }
            }
        } else {
            SetValueFromName(v_s ? v[0] : "");
        }
    }
}

void Rgba::SetRGBAFromARGB(std::string name) noexcept {
    if(name.empty()) {
        return;
    }
    name = StringUtils::ToUpperCase(name);

    if(std::size_t hash_loc = name.find_first_of('#'); hash_loc != std::string::npos) {
        name.replace(hash_loc, 1, "0X");
        std::size_t char_count = 0;
        uint32_t value_int = std::stoul(name, &char_count, 16);
        if(char_count == 10) { //0xAARRGGBB -> 0xRRGGBBAA
            const auto temp = Rgba(value_int);
            r = temp.g;
            g = temp.b;
            b = temp.a;
            a = temp.r;
        } else if(char_count == 8) { //0xRRGGBB
            SetRGBFromRawValue(value_int);
            a = 255;
        } else {
            /* DO NOTHING */
        }
    } else {
        const auto v = StringUtils::Split(name);
        const auto v_s = v.size();
        if(v_s > 1) {
            if(!(v_s < 3)) {
                r = static_cast<unsigned char>(std::stoi(v[0].data()));
                g = static_cast<unsigned char>(std::stoi(v[1].data()));
                b = static_cast<unsigned char>(std::stoi(v[2].data()));
                if(v_s > 3) {
                    a = static_cast<unsigned char>(std::stoi(v[0].data()));
                    r = static_cast<unsigned char>(std::stoi(v[1].data()));
                    g = static_cast<unsigned char>(std::stoi(v[2].data()));
                    b = static_cast<unsigned char>(std::stoi(v[3].data()));
                }
            }
        } else {
            SetValueFromName(v_s ? v[0] : "");
        }
    }
}

Rgba::Rgba(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha /*= 0xFF*/) noexcept
: r(red)
, g(green)
, b(blue)
, a(alpha) {
    /* DO NOTHING */
}

Rgba::Rgba(const uint32_t rawValue) {
    SetFromRawValue(rawValue);
}

void Rgba::SetAsBytes(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) noexcept {
    r = red;
    g = green;
    b = blue;
    a = alpha;
}

void Rgba::SetAsFloats(float normalized_red, float normalized_green, float normalized_blue, float normalized_alpha) noexcept {
    r = static_cast<unsigned char>(normalized_red * 255.0f);
    g = static_cast<unsigned char>(normalized_green * 255.0f);
    b = static_cast<unsigned char>(normalized_blue * 255.0f);
    a = static_cast<unsigned char>(normalized_alpha * 255.0f);
}

std::tuple<float, float, float, float> Rgba::GetAsFloats() const noexcept {
    return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

void Rgba::ScaleRGB(float scale) noexcept {
    const auto scaled_red = static_cast<float>(r) * scale;
    const auto scaled_green = static_cast<float>(g) * scale;
    const auto scaled_blue = static_cast<float>(b) * scale;
    r = static_cast<unsigned char>(std::clamp(scaled_red, 0.0f, 255.0f));
    g = static_cast<unsigned char>(std::clamp(scaled_green, 0.0f, 255.0f));
    b = static_cast<unsigned char>(std::clamp(scaled_blue, 0.0f, 255.0f));
}

void Rgba::ScaleAlpha(float scale) noexcept {
    const auto scaled_alpha = static_cast<float>(a) * scale;
    a = static_cast<unsigned char>(std::clamp(scaled_alpha, 0.0f, 255.0f));
}

void Rgba::Invert() noexcept {
    InvertRGB();
    InvertAlpha();
}

void Rgba::InvertRGB() noexcept {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
}

void Rgba::InvertAlpha() noexcept {
    a = 255 - a;
}

uint32_t Rgba::GetAsRawValue() const noexcept {
    return static_cast<uint32_t>(((static_cast<uint32_t>(r) << 24) & 0xFF000000u)
                                 | ((static_cast<uint32_t>(g) << 16) & 0x00FF0000u)
                                 | ((static_cast<uint32_t>(b) << 8) & 0x0000FF00u)
                                 | ((static_cast<uint32_t>(a) << 0) & 0x000000FFu));
}

void Rgba::SetFromRawValue(uint32_t value) noexcept {
    SetRGBAFromRawValue(value);
}

void Rgba::SetARGBFromRawValue(uint32_t value) noexcept {
    a = static_cast<uint8_t>((value & 0xFF000000u) >> 24);
    r = static_cast<uint8_t>((value & 0x00FF0000u) >> 16);
    g = static_cast<uint8_t>((value & 0x0000FF00u) >> 8);
    b = static_cast<uint8_t>((value & 0x000000FFu) >> 0);
}

void Rgba::SetRGBAFromRawValue(uint32_t value) noexcept {
    r = static_cast<uint8_t>((value & 0xFF000000u) >> 24);
    g = static_cast<uint8_t>((value & 0x00FF0000u) >> 16);
    b = static_cast<uint8_t>((value & 0x0000FF00u) >> 8);
    a = static_cast<uint8_t>((value & 0x000000FFu) >> 0);
}

void Rgba::SetRGBFromRawValue(uint32_t value) noexcept {
    r = static_cast<uint8_t>((value & 0xff0000u) >> 16);
    g = static_cast<uint8_t>((value & 0x00ff00u) >> 8);
    b = static_cast<uint8_t>((value & 0x0000ffu) >> 0);
}

void Rgba::SetFromFloats(std::initializer_list<float> ilist) noexcept {
    const auto length = ilist.size();
    constexpr auto convertToUChar = [](auto&& v) -> unsigned char {
        const auto clamped_v = std::clamp(std::forward<decltype(v)>(std::remove_pointer_t<decltype(v)>(std::decay_t<decltype(v)>(v))), 0.0f, 1.0f);
        return static_cast<unsigned char>(clamped_v * 255.0f);
    };
    switch(length) {
    case 1: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*std::begin(ilist));
        b = convertToUChar(*std::begin(ilist));
        a = convertToUChar(*std::begin(ilist));
        break;
    }
    case 2: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        break;
    }
    case 3: {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        b = convertToUChar(*(std::next(std::next(std::begin(ilist)))));
        break;
    }
    default: {
        /* DO NOTHING */
        break;
    }
    }
    if(length >= 4) {
        r = convertToUChar(*std::begin(ilist));
        g = convertToUChar(*(std::next(std::begin(ilist))));
        b = convertToUChar(*(std::next(std::next(std::begin(ilist)))));
        a = convertToUChar(*(std::next(std::next(std::next(std::begin(ilist))))));
    }
}

void Rgba::SetValueFromName(std::string name) noexcept {
    name = StringUtils::ToUpperCase(name);
    if(name == "WHITE") {
        SetFromRawValue(Rgba::White.GetAsRawValue());
    } else if(name == "BLACK") {
        SetFromRawValue(Rgba::Black.GetAsRawValue());
    } else if(name == "RED") {
        SetFromRawValue(Rgba::Red.GetAsRawValue());
    } else if(name == "GREEN") {
        SetFromRawValue(Rgba::Green.GetAsRawValue());
    } else if(name == "FORESTGREEN") {
        SetFromRawValue(Rgba::ForestGreen.GetAsRawValue());
    } else if(name == "NAVYBLUE") {
        SetFromRawValue(Rgba::NavyBlue.GetAsRawValue());
    } else if(name == "CYAN") {
        SetFromRawValue(Rgba::Cyan.GetAsRawValue());
    } else if(name == "YELLOW") {
        SetFromRawValue(Rgba::Yellow.GetAsRawValue());
    } else if(name == "MAGENTA") {
        SetFromRawValue(Rgba::Magenta.GetAsRawValue());
    } else if(name == "ORANGE") {
        SetFromRawValue(Rgba::Orange.GetAsRawValue());
    } else if(name == "GREY") {
        SetFromRawValue(Rgba::Grey.GetAsRawValue());
    } else if(name == "GRAY") {
        SetFromRawValue(Rgba::Gray.GetAsRawValue());
    } else if(name == "LIGHTGREY") {
        SetFromRawValue(Rgba::LightGrey.GetAsRawValue());
    } else if(name == "LIGHTGRAY") {
        SetFromRawValue(Rgba::LightGray.GetAsRawValue());
    } else if(name == "DARKGREY") {
        SetFromRawValue(Rgba::DarkGrey.GetAsRawValue());
    } else if(name == "DARKGRAY") {
        SetFromRawValue(Rgba::DarkGray.GetAsRawValue());
    } else if(name == "OLIVE") {
        SetFromRawValue(Rgba::Olive.GetAsRawValue());
    } else if(name == "SKYBLUE") {
        SetFromRawValue(Rgba::SkyBlue.GetAsRawValue());
    } else if(name == "MIDNIGHTBLUE") {
        SetFromRawValue(Rgba::MidnightBlue.GetAsRawValue());
    } else if(name == "LIME") {
        SetFromRawValue(Rgba::Lime.GetAsRawValue());
    } else if(name == "TEAL") {
        SetFromRawValue(Rgba::Teal.GetAsRawValue());
    } else if(name == "TURQUOISE") {
        SetFromRawValue(Rgba::Turquoise.GetAsRawValue());
    } else if(name == "PERIWINKLE") {
        SetFromRawValue(Rgba::Periwinkle.GetAsRawValue());
    } else if(name == "NORMALZ") {
        SetFromRawValue(Rgba::NormalZ.GetAsRawValue());
    } else if(name == "NOALPHA") {
        SetFromRawValue(Rgba::NoAlpha.GetAsRawValue());
    } else if(name == "PINK") {
        SetFromRawValue(Rgba::Pink.GetAsRawValue());
    } else if(name == "BLUE") {
        SetFromRawValue(Rgba::Blue.GetAsRawValue());
    } else if(name == "VIOLET") {
        SetFromRawValue(Rgba::Violet.GetAsRawValue());
    } else if(name == "TAUPE") {
        SetFromRawValue(Rgba::Taupe.GetAsRawValue());
    } else if(name == "UMBER") {
        SetFromRawValue(Rgba::Umber.GetAsRawValue());
    } else if(name == "BURNTUMBER") {
        SetFromRawValue(Rgba::BurntUmber.GetAsRawValue());
    } else if(name == "SIENNA") {
        SetFromRawValue(Rgba::Sienna.GetAsRawValue());
    } else if(name == "RAWSIENNA") {
        SetFromRawValue(Rgba::RawSienna.GetAsRawValue());
    }
}

bool Rgba::IsRgbEqual(const Rgba& rhs) const noexcept {
    return r == rhs.r && g == rhs.g && b == rhs.b;
}

bool Rgba::operator<(const Rgba& rhs) const noexcept {
    return GetAsRawValue() < rhs.GetAsRawValue();
}

Rgba operator+(Rgba lhs, const Rgba& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

Rgba& Rgba::operator+=(const Rgba& rhs) noexcept {
    int r_int = r + rhs.r;
    int g_int = g + rhs.g;
    int b_int = b + rhs.b;
    int a_int = a + rhs.a;
    r = static_cast<unsigned char>(std::clamp(r_int, 0, 255));
    g = static_cast<unsigned char>(std::clamp(g_int, 0, 255));
    b = static_cast<unsigned char>(std::clamp(b_int, 0, 255));
    a = static_cast<unsigned char>(std::clamp(a_int, 0, 255));
    return *this;
}

Rgba operator-(Rgba lhs, const Rgba& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

Rgba& Rgba::operator-=(const Rgba& rhs) noexcept {
    int r_int = r - rhs.r;
    int g_int = g - rhs.g;
    int b_int = b - rhs.b;
    int a_int = a - rhs.a;
    r = static_cast<unsigned char>(std::clamp(r_int, 0, 255));
    g = static_cast<unsigned char>(std::clamp(g_int, 0, 255));
    b = static_cast<unsigned char>(std::clamp(b_int, 0, 255));
    a = static_cast<unsigned char>(std::clamp(a_int, 0, 255));
    return *this;
}

Rgba& Rgba::operator++() noexcept {
    const auto raw = GetAsRawValue();
    SetFromRawValue(raw + 1);
    return *this;
}
Rgba Rgba::operator++(int) noexcept {
    Rgba temp(*this);
    operator++();
    return temp;
}

Rgba& Rgba::operator--() noexcept {
    const auto raw = GetAsRawValue();
    SetFromRawValue(raw - 1);
    return *this;
}
Rgba Rgba::operator--(int) noexcept {
    Rgba temp(*this);
    operator--();
    return temp;
}

bool Rgba::operator!=(const Rgba& rhs) const noexcept {
    return !(*this == rhs);
}

bool Rgba::operator==(const Rgba& rhs) const noexcept {
    return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}
