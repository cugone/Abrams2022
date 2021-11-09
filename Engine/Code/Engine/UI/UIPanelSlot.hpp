#pragma once

#include "Engine/Math/AABB2.hpp"

class UIPanel;
class UIElement;

struct UIPanelSlot {
    UIElement* content{nullptr};
    UIPanel* parent{nullptr};
    explicit UIPanelSlot(UIElement* content = nullptr, UIPanel* parent = nullptr)
    : content(content)
    , parent(parent) {
    }
    virtual ~UIPanelSlot() = default;
    virtual void CalcPivot() = 0;
    [[nodiscard]] virtual Vector2 CalcPosition() const = 0;
};

struct UINullPanelSlot : public UIPanelSlot {
    virtual ~UINullPanelSlot() = default;
    void CalcPivot() override{};
    [[nodiscard]] Vector2 CalcPosition() const override {
        return {};
    };
};

