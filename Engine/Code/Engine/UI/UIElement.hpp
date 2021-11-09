#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Matrix4.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/UI/UIPanelSlot.hpp"
#include "Engine/UI/UITypes.hpp"

class UIPanel;
struct UIPanelSlot;

class UIElement {
public:
    explicit UIElement(UIPanel* parent = nullptr);
    virtual ~UIElement() = 0;

    virtual void Update(TimeUtils::FPSeconds deltaSeconds);
    virtual void Render() const;
    virtual void DebugRender() const;
    virtual void EndFrame();
    void SetBorderColor(const Rgba& color);
    void SetBackgroundColor(const Rgba& color);
    void SetPivotColor(const Rgba& color);
    void SetDebugColors(const Rgba& edge, const Rgba& fill, const Rgba& pivot = Rgba::Red);

    [[nodiscard]] const Vector4& GetPosition() const;
    virtual void SetPosition(const Vector4& position);
    virtual void SetPositionOffset(const Vector2& offset);
    virtual void SetPositionRatio(const Vector2& ratio);

    [[nodiscard]] virtual Vector4 CalcDesiredSize() const noexcept = 0;

    void SetPivot(const Vector2& pivotPosition);
    [[nodiscard]] const Vector2& GetPivot() const;
    void SetPivot(const UIPivotPosition& pivotPosition);

    void SetOrientationDegrees(float value);
    void SetOrientationRadians(float value);
    [[nodiscard]] float GetOrientationDegrees() const;
    [[nodiscard]] float GetOrientationRadians() const;

    [[nodiscard]] bool HasParent() const;
    [[nodiscard]] AABB2 GetParentBounds() const noexcept;

    [[nodiscard]] UIPanel* GetParent() const noexcept;

    [[nodiscard]] bool IsHidden() const;
    [[nodiscard]] bool IsVisible() const;
    void Hide();
    void Show();
    void SetHidden(bool hidden = true);
    void ToggleHidden();
    void ToggleVisibility();

    [[nodiscard]] bool IsEnabled() const;
    [[nodiscard]] bool IsDisabled() const;
    void Enable();
    void Disable();
    void SetEnabled(bool enabled = true);
    void ToggleEnabled();

    [[nodiscard]] const std::string& GetName() const;
    [[nodiscard]] std::string& GetName();

    void RemoveSelf();

    [[nodiscard]] bool HasSlot() const noexcept;
    void ResetSlot() noexcept;
    void SetSlot(UIPanelSlot* newSlot) noexcept;
    [[nodiscard]] const UIPanelSlot* const GetSlot() const noexcept;
    [[nodiscard]] UIPanelSlot* GetSlot() noexcept;

protected:
    [[nodiscard]] Vector2 CalcLocalPosition() const;
    [[nodiscard]] Vector2 CalcLocalScale() const;
    [[nodiscard]] float CalcLocalRotationRadians() const;
    [[nodiscard]] float CalcLocalRotationDegrees() const;
    [[nodiscard]] float CalcWorldRotationRadians() const;
    [[nodiscard]] float CalcWorldRotationDegrees() const;

    [[nodiscard]] Vector2 CalcRelativePosition() const;
    [[nodiscard]] Vector2 CalcRelativePosition(const Vector2& position) const;

    void CalcBounds() noexcept;
    void CalcBoundsAndPivot() noexcept;
    [[nodiscard]] AABB2 CalcBoundsRelativeToParent() const noexcept;

    [[nodiscard]] AABB2 AlignBoundsToContainer(AABB2 bounds, AABB2 container, const Vector2& alignment) const noexcept;
    [[nodiscard]] AABB2 CalcRelativeBounds() const noexcept;
    [[nodiscard]] AABB2 CalcAbsoluteBounds() const noexcept;
    [[nodiscard]] AABB2 CalcAlignedAbsoluteBounds() const noexcept;
    [[nodiscard]] AABB2 CalcLocalBounds() const noexcept;

    [[nodiscard]] Matrix4 GetLocalTransform() const noexcept;
    [[nodiscard]] Matrix4 GetWorldTransform() const noexcept;
    [[nodiscard]] Matrix4 GetParentWorldTransform() const noexcept;

    void DirtyElement(UIInvalidateElementReason reason = UIInvalidateElementReason::Any);
    [[nodiscard]] bool IsDirty(UIInvalidateElementReason reason = UIInvalidateElementReason::Any) const;
    [[nodiscard]] bool IsParent() const;
    [[nodiscard]] bool IsChild() const;

    void DebugRenderBoundsAndPivot() const;
    void DebugRenderPivot() const;
    void DebugRenderBounds() const;

    [[nodiscard]] AABB2 GetParentLocalBounds() const;
    [[nodiscard]] AABB2 GetParentRelativeBounds() const;

    [[nodiscard]] AABB2 GetBounds(const AABB2& parent, const Vector4& anchors, const Vector4& offsets) const noexcept;
    [[nodiscard]] Vector2 GetSmallestOffset(AABB2 a, AABB2 b) const noexcept;
    [[nodiscard]] AABB2 MoveToBestFit(const AABB2& obj, const AABB2& container) const noexcept;

    [[nodiscard]] float GetAspectRatio() const noexcept;
    [[nodiscard]] float GetInvAspectRatio() const noexcept;

    [[nodiscard]] Vector2 GetTopLeft() const noexcept;
    [[nodiscard]] Vector2 GetTopRight() const noexcept;
    [[nodiscard]] Vector2 GetBottomLeft() const noexcept;
    [[nodiscard]] Vector2 GetBottomRight() const noexcept;

    std::string _name{};
    Rgba _fill_color{Rgba::NoAlpha};
    Rgba _edge_color{Rgba::White};

    AABB2 _bounds{};

private:
    [[nodiscard]] float GetParentOrientationRadians() const;
    [[nodiscard]] float GetParentOrientationDegrees() const;

    static inline UINullPanelSlot s_NullPanelSlot{};

    Vector4 _position{};
    Vector2 _pivot{};

    Rgba _pivot_color{Rgba::Red};
    UIPanelSlot* _slot{&s_NullPanelSlot};
    float _orientationRadians{0.0f};
    UIInvalidateElementReason _dirty_reason{UIInvalidateElementReason::None};
    bool _hidden{false};
    bool _enabled{true};

    friend class UIPanel;
    friend struct UIPanelSlot;
};

