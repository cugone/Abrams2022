#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/UI/UIElement.hpp"

#include <memory>

class AnimatedSprite;

class UIPanel;

class UIPictureBox : public UIElement {
public:
    explicit UIPictureBox(UIPanel* parent = nullptr);
    explicit UIPictureBox(const XMLElement& elem, UIPanel* parent = nullptr);
    virtual ~UIPictureBox() = default;

    void SetImage(std::unique_ptr<AnimatedSprite> sprite) noexcept;
    [[nodiscard]] const AnimatedSprite* const GetImage() const noexcept;
    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render() const override;
    virtual void DebugRender() const override;

    [[nodiscard]] Vector4 CalcDesiredSize() const noexcept override;

protected:
private:
    [[nodiscard]] bool LoadFromXml(const XMLElement& elem) noexcept;

    std::unique_ptr<AnimatedSprite> _sprite{};
};

