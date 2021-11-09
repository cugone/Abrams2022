#pragma once

#include "Engine/RHI/RHITypes.hpp"

class Material;

struct DrawInstruction {
    PrimitiveType type{PrimitiveType::None};
    std::size_t indexStart{0u};
    std::size_t indexCount{0u};
    std::size_t baseVertexLocation{0u};
    std::size_t count{1u};
    Material* material{};
    [[nodiscard]] bool operator==(const DrawInstruction& rhs);
    [[nodiscard]] bool operator!=(const DrawInstruction& rhs);
};
