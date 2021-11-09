#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/UI/UIElement.hpp"
#include "Engine/UI/UIPanelSlot.hpp"
#include "Engine/UI/UIWidget.hpp"

#include <memory>
#include <vector>

class Renderer;

class UIPanel : public UIElement {
public:
    explicit UIPanel(UIWidget* owner);
    UIPanel(UIPanel&& other) = default;
    UIPanel& operator=(UIPanel&& rhs) = default;
    UIPanel(const UIPanel& other) = delete;
    UIPanel& operator=(UIPanel& other) = delete;
    virtual ~UIPanel() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const override;
    void DebugRender() const override;
    void EndFrame() override;
    virtual UIPanelSlot* AddChild(UIElement* child) = 0;
    virtual UIPanelSlot* AddChildAt(UIElement* child, std::size_t index) = 0;
    virtual UIPanelSlot* AddChildFromXml(const XMLElement& elem, UIElement* child) = 0;
    virtual UIPanelSlot* AddChildFromXml(const XMLElement& elem, UIElement* child, std::size_t index) = 0;
    virtual void RemoveChild(UIElement* child) = 0;
    virtual void RemoveAllChildren() = 0;

    [[nodiscard]] const UIWidget* const GetOwningWidget() const noexcept;
    void SetOwningWidget(UIWidget* owner) noexcept;

    [[nodiscard]] Vector4 CalcDesiredSize() const noexcept override;

    void DebugRenderBottomUp() const;
    void DebugRenderTopDown() const;
    void DebugRenderChildren() const;

protected:
    [[nodiscard]] virtual AABB2 CalcChildrenDesiredBounds() const = 0;
    virtual void ArrangeChildren() noexcept = 0;
    [[nodiscard]] virtual bool LoadFromXml(const XMLElement& elem) noexcept = 0;
    virtual void UpdateChildren(TimeUtils::FPSeconds);
    virtual void RenderChildren() const;
    virtual void SortChildren();

    void CalcBoundsForChildren() noexcept;
    void CalcBoundsForMeThenMyChildren() noexcept;
    void CalcBoundsMyChildrenThenMe() noexcept;

    [[nodiscard]] virtual bool CanHaveManyChildren() const noexcept;
    std::vector<std::shared_ptr<UIPanelSlot>> _slots{};

private:
    UIWidget* _owner{};
};

