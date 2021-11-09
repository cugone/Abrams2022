#include "Engine/UI/UICanvas.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/RHI/RHIOutput.hpp"
#include "Engine/Renderer/DepthStencilState.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Texture2D.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include <memory>
#include <sstream>

UICanvas::UICanvas(UIWidget* owner)
: UIPanel(owner)
{
    const auto&& [dimensions, aspect_ratio] = CalcDimensionsAndAspectRatio();
    const auto desired_size = CalcDesiredSize();
    _bounds.mins = desired_size.GetXY();
    _bounds.maxs = desired_size.GetZW();

    auto desc = DepthStencilDesc{};
    desc.stencil_enabled = true;
    desc.stencil_testFront = ComparisonFunction::Equal;
    ServiceLocator::get<IRendererService>().CreateAndRegisterDepthStencilStateFromDepthStencilDescription("UIDepthStencil", desc);
}

UICanvas::UICanvas(UIWidget* owner, const XMLElement& elem)
: UIPanel(owner)
{
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Canvas constructor failed to load.");
}

void UICanvas::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(IsDisabled()) {
        return;
    }
    UpdateChildren(deltaSeconds);
}

void UICanvas::Render() const {
    if(IsHidden()) {
        return;
    }
    auto&& renderer = ServiceLocator::get<IRendererService>();
    const auto old_camera = renderer.GetCamera();
    SetupMVPFromTargetAndCamera();
    RenderChildren();
    renderer.SetCamera(old_camera);
}

void UICanvas::SetupMVPFromTargetAndCamera() const {
    SetupMVPFromViewportAndCamera();
}

void UICanvas::SetupMVPFromViewportAndCamera() const {
    auto&& renderer = ServiceLocator::get<IRendererService>();
    renderer.ResetModelViewProjection();
    const auto& vp = renderer.GetCurrentViewport();
    const auto target_dims = Vector2(vp.width, vp.height);
    const auto leftBottom = Vector2(0.0f, 1.0f) * target_dims;
    const auto rightTop = Vector2(1.0f, 0.0f) * target_dims;
    const auto nearFar = Vector2{0.0f, 1.0f};
    const auto& [dimensions, aspect_ratio] = CalcDimensionsAndAspectRatio();
    _camera.SetupView(leftBottom, rightTop, nearFar, aspect_ratio);
    const auto view_extents = Vector2{rightTop.x - leftBottom.x, leftBottom.y - rightTop.y};
    const auto view_half_extents = Vector2{view_extents * 0.5f};
    _camera.SetPosition(view_half_extents);
    renderer.SetCamera(_camera);
    renderer.SetModelMatrix(GetWorldTransform());
}

void UICanvas::DebugRender() const {
    const auto& target = _camera.GetRenderTarget();
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetRenderTarget(target.color_target, target.depthstencil_target);
    renderer.DisableDepth();
    DebugRenderBottomUp();
    renderer.EnableDepth();
    renderer.SetRenderTarget();
    renderer.SetMaterial(nullptr);
}

void UICanvas::EndFrame() {
    UIPanel::EndFrame();
    if(IsDirty(UIInvalidateElementReason::Layout)) {
        ReorderAllChildren();
    }
}

const Camera2D& UICanvas::GetUICamera() const {
    return _camera;
}

void UICanvas::UpdateChildren(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& slot : _slots) {
        if(auto* child = slot->content) {
            child->Update(deltaSeconds);
        }
    }
}

void UICanvas::RenderChildren() const {
    for(auto& slot : _slots) {
        if(auto* child = slot->content) {
            child->Render();
        }
    }
}

Vector4 UICanvas::AnchorTextToAnchorValues(const std::string& text) noexcept {
    const auto values = StringUtils::Split(text, '/');
    GUARANTEE_OR_DIE(values.size() == 2, "UI Anchor Text must be exactly two values separated by a '/'");
    Vector4 anchors{0.5f, 0.5f, 0.5f, 0.5f};
    if(values[0] == "left") {
        anchors.x = 0.0f;
    } else if(values[0] == "center") {
        anchors.x = 0.5f;
    } else if(values[0] == "right") {
        anchors.x = 1.0f;
    } else if(values[0] == "stretchH") {
        if(values[1] == "top") {
            anchors.y = 0.0f;
            return anchors;
        } else if(values[1] == "center") {
            anchors.y = 0.5f;
            return anchors;
        } else if(values[1] == "bottom") {
            anchors.y = 1.0f;
            return anchors;
        } else {
            ERROR_AND_DIE("Ill-formed anchor values.");
        }
    } else if(values[0] == "stretchV") {
        if(values[1] == "left") {
            anchors.y = 0.0f;
            return anchors;
        } else if(values[1] == "center") {
            anchors.y = 0.5f;
            return anchors;
        } else if(values[1] == "right") {
            anchors.y = 1.0f;
            return anchors;
        } else {
            ERROR_AND_DIE("Ill-formed anchor values.");
        }
    } else {
        if(values[0] == "stretch" && values[1] == "both") {
            return Vector4{0.0f, 0.0f, 1.0f, 1.0f};
        }
    }
    if(values[1] == "top") {
        anchors.y = 0.0f;
        return anchors;
    } else if(values[1] == "center") {
        anchors.y = 0.5f;
        return anchors;
    } else if(values[1] == "bottom") {
        anchors.y = 1.0f;
        return anchors;
    } else {
        ERROR_AND_DIE("Ill-formed anchor values.");
    }
}

UICanvasSlot* UICanvas::AddChild(UIElement* child) {
    DirtyElement(UIInvalidateElementReason::Layout);
    auto newSlot = std::make_shared<UICanvasSlot>(child, this);
    auto* ptr = newSlot.get();
    _slots.emplace_back(newSlot);
    child->SetSlot(ptr);
    return ptr;
}

UICanvasSlot* UICanvas::AddChildAt(UIElement* child, std::size_t index) {
    DirtyElement(UIInvalidateElementReason::Layout);
    auto newSlot = std::make_shared<UICanvasSlot>(child, this);
    CalcBoundsForMeThenMyChildren();
    auto* ptr = newSlot.get();
    _slots[index] = std::move(newSlot);
    if(IsDirty(UIInvalidateElementReason::Layout)) {
        ReorderAllChildren();
    }
    return ptr;
}

UICanvasSlot* UICanvas::AddChildFromXml(const XMLElement& elem, UIElement* child) {
    DirtyElement(UIInvalidateElementReason::Layout);
    auto newSlot = std::make_shared<UICanvasSlot>(elem, child, this);
    auto* ptr = newSlot.get();
    _slots.emplace_back(newSlot);
    child->SetSlot(ptr);
    if(IsDirty(UIInvalidateElementReason::Layout)) {
        ReorderAllChildren();
    }
    return ptr;
}

UICanvasSlot* UICanvas::AddChildFromXml(const XMLElement& elem, UIElement* child, std::size_t index) {
    DirtyElement(UIInvalidateElementReason::Layout);
    auto newSlot = std::make_shared<UICanvasSlot>(elem, child, this);
    CalcBoundsForMeThenMyChildren();
    auto* ptr = newSlot.get();
    _slots[index] = std::move(newSlot);
    if(IsDirty(UIInvalidateElementReason::Layout)) {
        ReorderAllChildren();
    }
    return ptr;
}

void UICanvas::RemoveChild(UIElement* child) {
    DirtyElement(UIInvalidateElementReason::Any);
    _slots.erase(
    std::remove_if(std::begin(_slots), std::end(_slots),
                   [child](const decltype(_slots)::value_type& c) {
                       return child == c->content;
                   }),
    std::end(_slots));
    ReorderAllChildren();
    CalcBoundsForMeThenMyChildren();
}

void UICanvas::RemoveAllChildren() {
    DirtyElement(UIInvalidateElementReason::Any);
    _slots.clear();
    _slots.shrink_to_fit();
    CalcBoundsForMeThenMyChildren();
}

Vector4 UICanvas::CalcDesiredSize() const noexcept {
    const auto childBounds = CalcChildrenDesiredBounds();
    return Vector4{childBounds.mins, childBounds.maxs};
}

AABB2 UICanvas::CalcChildrenDesiredBounds() const {
    AABB2 result;
    for(const auto& slot : _slots) {
        const auto desired_size = slot->content->CalcDesiredSize();
        result.StretchToIncludePoint(slot->CalcPosition());
        result.StretchToIncludePoint(desired_size.GetZW());
    }
    return result;
}

void UICanvas::ArrangeChildren() noexcept {
    /* DO NOTHING */
}

std::pair<Vector2, float> UICanvas::CalcDimensionsAndAspectRatio() const {
    const auto& viewport = ServiceLocator::get<IRendererService>().GetCurrentViewport();
    const auto viewport_dims = Vector2{viewport.width, viewport.height};

    const auto target_AR = viewport_dims.x / viewport_dims.y;
    auto dims = Vector2::Zero;
    if(target_AR <= 1.0f) {
        dims.x = viewport_dims.x;
        dims.y = target_AR * viewport_dims.x;
    } else {
        dims.x = target_AR * viewport_dims.y;
        dims.y = viewport_dims.y;
    }
    return std::make_pair(dims, dims.x / dims.y);
}

AABB2 UICanvas::CalcAlignedAbsoluteBounds() const noexcept {
    AABB2 parent_bounds = GetParentLocalBounds();
    auto ratio = GetPosition().GetXY();
    AABB2 alignedBounds = AlignBoundsToContainer(CalcBoundsRelativeToParent(), parent_bounds, ratio);

    auto unit = GetPosition().GetZW();
    Vector2 normalized_ratio = MathUtils::RangeMap(ratio, Vector2(0.0f, 1.0f), Vector2(-1.0f, 1.0f));
    Vector2 scaled_ratio = normalized_ratio * unit;
    Vector2 offset(scaled_ratio);

    alignedBounds.Translate(offset);

    return alignedBounds;
}

void UICanvas::ReorderAllChildren() {
    std::sort(std::begin(_slots), std::end(_slots),
              [](const std::shared_ptr<UIPanelSlot>& a, const std::shared_ptr<UIPanelSlot>& b) {
                  const auto aAsCs = std::dynamic_pointer_cast<UICanvasSlot>(a);
                  const auto bAsCs = std::dynamic_pointer_cast<UICanvasSlot>(b);
                  if(aAsCs && bAsCs) {
                      return aAsCs->zOrder < bAsCs->zOrder;
                  }
                  if(aAsCs || bAsCs) {
                      if(aAsCs) {
                          return aAsCs->zOrder < 0;
                      }
                      if(bAsCs) {
                          return bAsCs->zOrder < 0;
                      }
                  }
                  return false;
              });
}

bool UICanvas::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "canvas", "", "name", "canvas,label,panel,picturebox,button");
    _name = DataUtils::ParseXmlAttribute(elem, "name", _name);
    return true;
}

UICanvasSlot::UICanvasSlot(UIElement* content /*= nullptr*/, UIPanel* parent /*= nullptr*/)
: UIPanelSlot(content, parent) {
}

UICanvasSlot::UICanvasSlot(const XMLElement& elem,
                       UIElement* content /*= nullptr*/,
                       UIPanel* parent /*= nullptr*/)
: UIPanelSlot(content, parent) {
    LoadFromXml(elem);
}

void UICanvasSlot::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "slot", "", "", "", "anchors,position,size,alignment,autosize");
    const auto anchorValues = AABB2{UICanvas::AnchorTextToAnchorValues(DataUtils::ParseXmlAttribute(elem, "anchors", std::string{"center/center"}))};
    anchors = anchorValues;
    CalcPivot();
    autoSize = DataUtils::ParseXmlAttribute(elem, "autosize", autoSize);
    size = autoSize ? content->CalcDesiredSize().GetZW() : DataUtils::ParseXmlAttribute(elem, "size", Vector2::Zero);
    position = DataUtils::ParseXmlAttribute(elem, "position", Vector2{0.5f, 0.5f});
}

void UICanvasSlot::CalcPivot() {
    const auto desired_size = content->CalcDesiredSize();
    const auto parent_bounds = content->GetParentBounds();
    const auto pivot_position = MathUtils::CalcPointFromNormalizedPoint(content->GetPivot(), parent_bounds);
    this->size = desired_size.GetZW();
    content->SetPivot(pivot_position);
}

Vector2 UICanvasSlot::CalcPosition() const {
    return position;
}
