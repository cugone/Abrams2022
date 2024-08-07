#include "Engine/UI/UIElement.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Engine/UI/UIPanel.hpp"

#include <sstream>

UIElement::UIElement(UIPanel* parent /*= nullptr*/) {
    if(parent) {
        m_slot = parent->AddChild(this);
    }
}

UIElement::~UIElement() {
    RemoveSelf();
}

void UIElement::RemoveSelf() {
    if(m_slot && m_slot->parent) {
        m_slot->parent->RemoveChild(this);
        m_slot->parent = nullptr;
        m_slot->content = nullptr;
        m_slot = &s_NullPanelSlot;
    }
}

bool UIElement::HasSlot() const noexcept {
    return m_slot != &s_NullPanelSlot;
}

void UIElement::ResetSlot() noexcept {
    m_slot = &s_NullPanelSlot;
}

void UIElement::SetSlot(UIPanelSlot* newSlot) noexcept {
    m_slot = newSlot;
}

const UIPanelSlot* const UIElement::GetSlot() const noexcept {
    return m_slot;
}

UIPanelSlot* UIElement::GetSlot() noexcept {
    return m_slot;
}

void UIElement::SetBorderColor(const Rgba& color) {
    SetDebugColors(color, m_fill_color, m_pivot_color);
}

void UIElement::SetBackgroundColor(const Rgba& color) {
    SetDebugColors(m_edge_color, color, m_pivot_color);
}

void UIElement::SetPivotColor(const Rgba& color) {
    SetDebugColors(m_edge_color, m_fill_color, color);
}

void UIElement::SetDebugColors(const Rgba& edge, const Rgba& fill, const Rgba& pivot /*= Rgba::RED*/) {
    m_edge_color = edge;
    m_fill_color = fill;
    m_pivot_color = pivot;
}

Vector2 UIElement::CalcLocalPosition() const {
    AABB2 local_bounds = GetParentBounds();
    return MathUtils::CalcPointFromNormalizedPoint(m_position.GetXY(), local_bounds) + m_position.GetZW();
}

Vector2 UIElement::CalcRelativePosition(const Vector2& position) const {
    AABB2 parent_bounds = GetParentLocalBounds();
    return MathUtils::CalcPointFromNormalizedPoint(position, parent_bounds);
}

Vector2 UIElement::CalcRelativePosition() const {
    AABB2 parent_bounds = GetParentLocalBounds();
    return MathUtils::CalcPointFromNormalizedPoint(m_pivot, parent_bounds);
}

const Vector4& UIElement::GetPosition() const {
    return m_position;
}

void UIElement::SetPosition(const Vector4& position) {
    DirtyElement(UIInvalidateElementReason::Layout);
    m_position = position;
    CalcBounds();
}

void UIElement::SetPositionRatio(const Vector2& ratio) {
    UIElement::SetPosition(Vector4{ratio, m_position.GetZW()});
}

void UIElement::SetPositionOffset(const Vector2& offset) {
    UIElement::SetPosition(Vector4{m_position.GetXY(), offset});
}

void UIElement::SetPivot(const Vector2& pivotPosition) {
    DirtyElement(UIInvalidateElementReason::Layout);
    m_pivot = pivotPosition;
    CalcBounds();
}

void UIElement::SetPivot(const UIPivotPosition& pivotPosition) {
    switch(pivotPosition) {
    case UIPivotPosition::Center:
        SetPivot(Vector2(0.5f, 0.5f));
        break;
    case UIPivotPosition::TopLeft:
        SetPivot(Vector2(0.0f, 0.0f));
        break;
    case UIPivotPosition::Top:
        SetPivot(Vector2(0.5f, 0.0f));
        break;
    case UIPivotPosition::TopRight:
        SetPivot(Vector2(1.0f, 0.0f));
        break;
    case UIPivotPosition::Right:
        SetPivot(Vector2(1.0f, 0.5f));
        break;
    case UIPivotPosition::BottomRight:
        SetPivot(Vector2(1.0f, 1.0f));
        break;
    case UIPivotPosition::Bottom:
        SetPivot(Vector2(0.5f, 1.0f));
        break;
    case UIPivotPosition::BottomLeft:
        SetPivot(Vector2(0.0f, 1.0f));
        break;
    case UIPivotPosition::Left:
        SetPivot(Vector2(0.0f, 0.5f));
        break;
    default:
        const auto error_msg = std::format("{}: Unhandled pivot mode.", std::string{__FUNCTION__});
        ERROR_AND_DIE(error_msg.c_str());
        break;
    }
}

const Vector2& UIElement::GetPivot() const {
    return m_pivot;
}

void UIElement::Update(TimeUtils::FPSeconds /*deltaSeconds*/) {
    /* DO NOTHING */
}

void UIElement::Render() const {
    /* DO NOTHING */
}

void UIElement::DebugRender() const {
    DebugRenderBoundsAndPivot();
}

void UIElement::EndFrame() {
    /* DO NOTHING */
}

Matrix4 UIElement::GetLocalTransform() const noexcept {
    const auto T = Matrix4::CreateTranslationMatrix(CalcLocalPosition());
    const auto R = Matrix4::Create2DRotationMatrix(CalcLocalRotationRadians());
    const auto S = Matrix4::CreateScaleMatrix(CalcLocalScale());
    const auto M = Matrix4::MakeSRT(S, R, T);
    return M;
}

Vector2 UIElement::CalcLocalScale() const {
    const auto my_bounds = CalcLocalBounds();
    const auto parent_bounds = GetParentBounds();
    const auto parent_width = parent_bounds.maxs.x - parent_bounds.mins.x;
    const auto my_width = my_bounds.maxs.x - my_bounds.mins.x;
    const auto width_scale = my_width / parent_width;
    const auto parent_height = parent_bounds.maxs.y - parent_bounds.mins.y;
    const auto my_height = my_bounds.maxs.y - my_bounds.mins.y;
    const auto height_scale = my_height / parent_height;
    const auto* parent = GetParent();
    return parent ? Vector2(width_scale, height_scale) : Vector2::One;
}

Matrix4 UIElement::GetWorldTransform() const noexcept {
    return Matrix4::MakeRT(GetLocalTransform(), GetParentWorldTransform());
}

Matrix4 UIElement::GetParentWorldTransform() const noexcept {
    const auto* parent = GetParent();
    return parent ? parent->GetWorldTransform() : Matrix4::I;
}

void UIElement::DirtyElement(UIInvalidateElementReason reason /*= InvalidateElementReason::Any*/) {
    m_dirty_reason = reason;
}

void UIElement::DebugRenderBoundsAndPivot() const {
    DebugRenderBounds();
    DebugRenderPivot();
}

void UIElement::DebugRenderPivot() const {
    const auto world_transform = GetWorldTransform();
    const auto scale = world_transform.GetScale();
    const auto inv_scale_matrix = Matrix4::CalculateInverse(Matrix4::CreateScaleMatrix(Vector3(scale.x * 0.10f, scale.y * 0.10f, 1.0f)));
    const auto pivot_pos = MathUtils::CalcPointFromNormalizedPoint(m_pivot, m_bounds);
    const auto pivot_pos_matrix = Matrix4::CreateTranslationMatrix(pivot_pos);
    const auto transform = Matrix4::MakeSRT(inv_scale_matrix, world_transform, pivot_pos_matrix);
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetMaterial(renderer->GetMaterial("__2D"));
    renderer->SetModelMatrix(transform);
    renderer->DrawX2D(m_pivot_color);
}

void UIElement::DebugRenderBounds() const {
    const auto world_transform = GetWorldTransform();
    auto* renderer = ServiceLocator::get<IRendererService>();
    renderer->SetModelMatrix(world_transform);
    renderer->SetMaterial(renderer->GetMaterial("__2D"));
    renderer->DrawAABB2(m_edge_color, m_fill_color);
}

AABB2 UIElement::GetParentBounds() const noexcept {
    const auto* parent = GetParent();
    return parent ? parent->m_bounds : AABB2::Zero_to_One;
}

UIPanel* UIElement::GetParent() const noexcept {
    return m_slot->parent;
}

bool UIElement::IsHidden() const {
    return m_hidden;
}

bool UIElement::IsVisible() const {
    return !m_hidden;
}

void UIElement::Hide() {
    SetHidden();
}

void UIElement::Show() {
    SetHidden(false);
}

void UIElement::SetHidden(bool hidden /*= true*/) {
    m_hidden = hidden;
}

void UIElement::ToggleHidden() {
    m_hidden = !m_hidden;
}

void UIElement::ToggleVisibility() {
    ToggleHidden();
}

bool UIElement::IsEnabled() const {
    return m_enabled;
}

bool UIElement::IsDisabled() const {
    return !m_enabled;
}

void UIElement::Enable() {
    m_enabled = true;
}

void UIElement::Disable() {
    m_enabled = false;
}

void UIElement::SetEnabled(bool enabled /*= true*/) {
    m_enabled = enabled;
}

void UIElement::ToggleEnabled() {
    m_enabled = !m_enabled;
}

const std::string& UIElement::GetName() const {
    return m_name;
}

std::string& UIElement::GetName() {
    return m_name;
}

void UIElement::CalcBounds() noexcept {
    DirtyElement(UIInvalidateElementReason::Layout);
    const auto desired_size = this->CalcDesiredSize();
    m_bounds.mins = desired_size.GetXY();
    m_bounds.maxs = desired_size.GetZW();
}

void UIElement::CalcBoundsAndPivot() noexcept {
    DirtyElement(UIInvalidateElementReason::Layout);
    auto* const slot = GetSlot();
    CalcBounds();
    slot->CalcPivot();
}

AABB2 UIElement::CalcBoundsRelativeToParent() const noexcept {
    const auto* parent = GetParent();
    AABB2 parent_bounds = parent ? parent->CalcLocalBounds() : CalcLocalBounds();
    Vector2 parent_size = parent_bounds.CalcDimensions();

    Vector2 pivot_position = parent_bounds.mins + (parent_size * m_position.GetXY() + m_position.GetZW());

    AABB2 my_local_bounds = CalcLocalBounds();
    my_local_bounds.Translate(pivot_position);

    return my_local_bounds;
}

AABB2 UIElement::CalcRelativeBounds() const noexcept {
    Vector2 size = CalcDesiredSize().GetZW();
    Vector2 pivot_position = size * m_pivot;

    AABB2 bounds;
    bounds.StretchToIncludePoint(Vector2::Zero);
    bounds.StretchToIncludePoint(size);
    bounds.Translate(-pivot_position);
    return bounds;
}

AABB2 UIElement::CalcAbsoluteBounds() const noexcept {
    const auto size = CalcDesiredSize();
    AABB2 bounds;
    bounds.StretchToIncludePoint(Vector2::Zero);
    bounds.StretchToIncludePoint(size.GetZW());
    return CalcAlignedAbsoluteBounds();
}

AABB2 UIElement::AlignBoundsToContainer(AABB2 bounds, AABB2 container, const Vector2& alignment) const noexcept {
    Vector2 max_distance = MathUtils::CalcPointFromNormalizedPoint(alignment, bounds);
    Vector2 distance = MathUtils::CalcPointFromNormalizedPoint(alignment, container) + max_distance;
    bounds.Translate(distance);
    return bounds;
}

AABB2 UIElement::CalcAlignedAbsoluteBounds() const noexcept {
    AABB2 parent_bounds = GetParentLocalBounds();
    const auto ratio = m_position.GetXY();
    AABB2 alignedBounds = AlignBoundsToContainer(CalcBoundsRelativeToParent(), parent_bounds, ratio);

    const auto unit = m_position.GetZW();
    Vector2 normalized_ratio = MathUtils::RangeMap(ratio, Vector2(0.0f, 1.0f), Vector2(-1.0f, 1.0f));
    Vector2 scaled_ratio = normalized_ratio * unit;
    Vector2 offset(scaled_ratio);

    alignedBounds.Translate(offset);

    return alignedBounds;
}

AABB2 UIElement::CalcLocalBounds() const noexcept {
    return AABB2{CalcDesiredSize()};
}

bool UIElement::IsDirty(UIInvalidateElementReason reason /*= InvalidateElementReason::Any*/) const {
    return (m_dirty_reason & reason) == reason;
}

bool UIElement::IsParent() const {
    return !IsChild();
}

bool UIElement::IsChild() const {
    return GetParent() != nullptr;
}

AABB2 UIElement::GetParentLocalBounds() const {
    const auto* parent = GetParent();
    return parent ? parent->CalcLocalBounds() : AABB2(Vector2::Zero, m_bounds.CalcDimensions());
}

AABB2 UIElement::GetParentRelativeBounds() const {
    const auto* parent = GetParent();
    return parent ? parent->CalcBoundsRelativeToParent() : AABB2{0.0f, 0.0f, 0.0f, 0.0f};
}

AABB2 UIElement::GetBounds(const AABB2& parent, const Vector4& anchors, const Vector4& offsets) const noexcept {
    Vector2 boundMins = MathUtils::CalcPointFromNormalizedPoint(Vector2(anchors.x, anchors.y), parent) + Vector2(offsets.x, offsets.y);
    Vector2 boundMaxs = MathUtils::CalcPointFromNormalizedPoint(Vector2(anchors.z, anchors.w), parent) + Vector2(offsets.z, offsets.w);
    return AABB2(boundMins, boundMaxs);
}

Vector2 UIElement::GetSmallestOffset(AABB2 a, AABB2 b) const noexcept {
    const auto width = a.CalcDimensions().x;
    const auto height = a.CalcDimensions().y;
    const auto center = a.CalcCenter();
    b.AddPaddingToSides(-(width * 0.5f), -(height * 0.5f));
    Vector2 closestPoint = MathUtils::CalcClosestPoint(center, b);
    return closestPoint - center;
}

AABB2 UIElement::MoveToBestFit(const AABB2& obj, const AABB2& container) const noexcept {
    Vector2 offset = GetSmallestOffset(obj, container);
    return obj + offset;
}

float UIElement::GetAspectRatio() const noexcept {
    const auto dims = m_bounds.CalcDimensions();
    return dims.x / dims.y;
}

Vector2 UIElement::GetTopLeft() const noexcept {
    return m_bounds.mins;
}

Vector2 UIElement::GetTopRight() const noexcept {
    return Vector2{m_bounds.maxs.x, m_bounds.mins.y};
}

Vector2 UIElement::GetBottomLeft() const noexcept {
    return Vector2{m_bounds.mins.x, m_bounds.maxs.y};
}

Vector2 UIElement::GetBottomRight() const noexcept {
    return m_bounds.maxs;
}

bool UIElement::HasParent() const {
    return GetParent();
}

float UIElement::GetParentOrientationRadians() const {
    const auto* parent = GetParent();
    return parent ? parent->GetOrientationRadians() : 0.0f;
}

float UIElement::GetParentOrientationDegrees() const {
    const auto* parent = GetParent();
    return parent ? parent->GetOrientationDegrees() : 0.0f;
}

void UIElement::SetOrientationDegrees(float value) {
    m_orientationRadians = MathUtils::ConvertDegreesToRadians(value);
}

void UIElement::SetOrientationRadians(float value) {
    m_orientationRadians = value;
}

float UIElement::GetOrientationDegrees() const {
    return MathUtils::ConvertRadiansToDegrees(GetOrientationRadians());
}

float UIElement::GetOrientationRadians() const {
    return m_orientationRadians;
}

float UIElement::CalcLocalRotationDegrees() const {
    return MathUtils::ConvertRadiansToDegrees(GetOrientationDegrees());
}

float UIElement::CalcLocalRotationRadians() const {
    return GetOrientationRadians();
}

float UIElement::CalcWorldRotationRadians() const {
    return GetParentOrientationRadians() + GetOrientationRadians();
}

float UIElement::CalcWorldRotationDegrees() const {
    return GetParentOrientationDegrees() + GetOrientationDegrees();
}

float UIElement::GetInvAspectRatio() const noexcept {
    return 1.0f / GetAspectRatio();
}
