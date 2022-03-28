#include "Engine/UI/UIPanel.hpp"

#include "Engine/UI/UICanvas.hpp"
#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

UIPanel::UIPanel(UIWidget* owner)
: m_owner(owner) {
    /* DO NOTHING */
}

void UIPanel::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(IsDisabled()) {
        return;
    }
    UpdateChildren(deltaSeconds);
}

void UIPanel::Render() const {
    if(IsHidden()) {
        return;
    }
    if(0 < m_edge_color.a || 0 < m_fill_color.a) {
        DebugRenderBounds();
    }
    RenderChildren();
}

void UIPanel::DebugRender() const {
    DebugRenderBottomUp();
}

void UIPanel::EndFrame() {
    CalcBoundsForMeThenMyChildren();
}

const UIWidget* const UIPanel::GetOwningWidget() const noexcept {
    return m_owner;
}

void UIPanel::SetOwningWidget(UIWidget* owner) noexcept {
    m_owner = owner;
}

Vector4 UIPanel::CalcDesiredSize() const noexcept {
    return Vector4::Zero;
}

void UIPanel::DebugRenderBottomUp() const {
    DebugRenderBoundsAndPivot();
    DebugRenderChildren();
}

void UIPanel::DebugRenderTopDown() const {
    DebugRenderChildren();
    DebugRenderBoundsAndPivot();
}

void UIPanel::DebugRenderChildren() const {
    for(auto& slot : m_slots) {
        if(slot) {
            slot->content->DebugRender();
        }
    }
}

void UIPanel::SortChildren() {
}

void UIPanel::CalcBoundsForChildren() noexcept {
    for(auto& slot : m_slots) {
        if(slot && slot->content) {
            slot->content->CalcBounds();
        }
    }
}

void UIPanel::CalcBoundsForMeThenMyChildren() noexcept {
    CalcBounds();
    CalcBoundsForChildren();
}

void UIPanel::CalcBoundsMyChildrenThenMe() noexcept {
    CalcBoundsForChildren();
    CalcBounds();
}

bool UIPanel::CanHaveManyChildren() const noexcept {
    return true;
}

void UIPanel::UpdateChildren(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& slot : m_slots) {
        if(slot && slot->content) {
            slot->content->Update(deltaSeconds);
        }
    }
}

void UIPanel::RenderChildren() const {
    for(const auto& slot : m_slots) {
        if(slot && slot->content) {
            slot->content->Render();
        }
    }
}
