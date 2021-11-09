#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/UI/UIPanel.hpp"

class UICanvasSlot : public UIPanelSlot {
public:
    explicit UICanvasSlot(UIElement* content = nullptr,
                        UIPanel* parent = nullptr);
    explicit UICanvasSlot(const XMLElement& elem,
                        UIElement* content = nullptr,
                        UIPanel* parent = nullptr);
    void LoadFromXml(const XMLElement& elem);
    AABB2 anchors{};
    Vector2 position{};
    Vector2 size{};
    Vector2 alignment{};
    int zOrder{};
    bool autoSize{false};
    void CalcPivot() override;
    [[nodiscard]] Vector2 CalcPosition() const override;
};

class UICanvas : public UIPanel {
public:
    explicit UICanvas(UIWidget* owner);
    explicit UICanvas(UIWidget* owner, const XMLElement& elem);
    virtual ~UICanvas() = default;
    void Update(TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const override;
    void SetupMVPFromTargetAndCamera() const;
    void SetupMVPFromViewportAndCamera() const;
    void DebugRender() const override;
    void EndFrame() override;
    [[nodiscard]] const Camera2D& GetUICamera() const;

    void UpdateChildren(TimeUtils::FPSeconds deltaSeconds) override;
    void RenderChildren() const override;

    [[nodiscard]] static Vector4 AnchorTextToAnchorValues(const std::string& text) noexcept;

    UICanvasSlot* AddChild(UIElement* child) override;
    UICanvasSlot* AddChildAt(UIElement* child, std::size_t index) override;

    UICanvasSlot* AddChildFromXml(const XMLElement& elem, UIElement* child) override;
    UICanvasSlot* AddChildFromXml(const XMLElement& elem, UIElement* child, std::size_t index) override;

    void RemoveChild(UIElement* child) override;
    void RemoveAllChildren() override;

    [[nodiscard]] Vector4 CalcDesiredSize() const noexcept override;

protected:
    [[nodiscard]] AABB2 CalcChildrenDesiredBounds() const override;
    void ArrangeChildren() noexcept override;

private:
    void ReorderAllChildren();

    [[nodiscard]] bool LoadFromXml(const XMLElement& elem) noexcept;
    [[nodiscard]] std::pair<Vector2, float> CalcDimensionsAndAspectRatio() const;
    [[nodiscard]] AABB2 CalcAlignedAbsoluteBounds() const noexcept;

    mutable Camera2D _camera{};

    friend class UICanvasSlot;
    friend class UIWidget;
};
