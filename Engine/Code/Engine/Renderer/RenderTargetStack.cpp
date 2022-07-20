#include "Engine/Renderer/RenderTargetStack.hpp"

#include "Engine/Renderer/DepthStencilState.hpp"
#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

bool operator==(const RenderTargetStack::Node& lhs, const RenderTargetStack::Node& rhs) {
    return lhs.color_target == rhs.color_target && lhs.depthstencil_target == rhs.depthstencil_target && lhs.view_desc == rhs.view_desc;
}

bool operator!=(const RenderTargetStack::Node& lhs, const RenderTargetStack::Node& rhs) {
    return !(lhs == rhs);
}

[[nodiscard]] bool RenderTargetStack::empty() const {
    return m_stack.empty();
}

[[nodiscard]] std::size_t RenderTargetStack::size() const {
    return m_stack.size();
}

void RenderTargetStack::push(const RenderTargetStack::Node& node) noexcept {
    m_stack.push(node);
    const auto& top = m_stack.top();
    auto* rs = ServiceLocator::get<IRendererService>();
    rs->SetRenderTarget(top.color_target, top.depthstencil_target);
    const auto x = top.view_desc.x;
    const auto y = top.view_desc.y;
    const auto w = top.view_desc.width;
    const auto h = top.view_desc.height;
    rs->SetViewport(x, y, w, h);
}

void RenderTargetStack::push(RenderTargetStack::Node&& node) noexcept {
    m_stack.push(node);
    const auto& top = m_stack.top();
    auto* rs = ServiceLocator::get<IRendererService>();
    rs->SetRenderTarget(top.color_target, top.depthstencil_target);
    const auto x = top.view_desc.x;
    const auto y = top.view_desc.y;
    const auto w = top.view_desc.width;
    const auto h = top.view_desc.height;
    rs->SetViewport(x, y, w, h);
}

void RenderTargetStack::pop() noexcept {
    m_stack.pop();
    const auto& top = m_stack.top();
    auto* rs = ServiceLocator::get<IRendererService>();
    rs->SetRenderTarget(top.color_target, top.depthstencil_target);
    rs->ClearColor(Rgba::Black);
    rs->ClearDepthStencilBuffer();
    const auto x = top.view_desc.x;
    const auto y = top.view_desc.y;
    const auto w = top.view_desc.width;
    const auto h = top.view_desc.height;
    rs->SetViewport(x, y, w, h);
}

[[nodiscard]] RenderTargetStack::Node& RenderTargetStack::top() noexcept {
    return m_stack.top();
}

[[nodiscard]] const RenderTargetStack::Node& RenderTargetStack::top() const noexcept {
    return m_stack.top();
}
