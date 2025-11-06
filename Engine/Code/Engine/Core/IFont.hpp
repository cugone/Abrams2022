#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vector2.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

class Material;

namespace a2de {

class IFont {
public:
    virtual ~IFont() noexcept = default;

    [[nodiscard]] virtual float CalculateTextWidth(const std::string& text, float scale = 1.0f) const noexcept = 0;
    [[nodiscard]] virtual float CalculateTextHeight(float scale = 1.0f) const noexcept = 0;

    [[nodiscard]] virtual Vector2 CalculateTextDimensions(const std::string& text, float scale = 1.0f) const noexcept = 0;
    [[nodiscard]] virtual AABB2 CalculateTextArea(const std::string& text, float scale = 1.0f) const noexcept = 0;

    [[nodiscard]] virtual float GetLineHeight() const noexcept = 0;
    [[nodiscard]] virtual float GetLineHeightAsUV() const noexcept = 0;

    [[nodiscard]] virtual bool LoadFromFile(std::filesystem::path filepath) noexcept = 0;
    [[nodiscard]] virtual bool LoadFromBuffer(const std::vector<uint8_t>& buffer) noexcept = 0;

    [[nodiscard]] virtual Material* GetMaterial() const noexcept = 0;
    virtual void SetMaterial(Material* mat) noexcept = 0;

    [[nodiscard]] virtual const std::string& GetName() const noexcept = 0;
    [[nodiscard]] virtual const std::filesystem::path& GetFilePath() const noexcept = 0;

    [[nodiscard]] virtual int GetKerningValue(unsigned long first, unsigned long second) const noexcept = 0;

    [[nodiscard]] virtual AABB2 GetGlyphUVs(int c) const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetGlyphOffsets(int c) const noexcept = 0;
    [[nodiscard]] virtual Vector2 GetGlyphDimensions(int c) const noexcept = 0;
    [[nodiscard]] virtual int GetGlyphAdvance(int c) const noexcept = 0;

    [[nodiscard]] virtual int GetEmSize() const noexcept = 0;

    [[nodiscard]] virtual bool IsLoaded() const noexcept = 0;
};

} // namespace a2de
