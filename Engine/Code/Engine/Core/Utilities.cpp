#include "Engine/Core/Utilities.hpp"

bool Utils::FlipFlop() noexcept {
    static bool value = true;
    bool is_true = value;
    value = !value;
    return is_true;
}
