#pragma once

#include "Engine/RHI/RHITypes.hpp"

#include <stack>
#include <vector>

class DepthStencilState;
class Texture;

class RenderTargetStack {
public:
    struct Node {
        Texture* color_target{};
        Texture* depthstencil_target{};
        ViewportDesc view_desc{};
    };

    RenderTargetStack() noexcept = default;
    ~RenderTargetStack() = default;

    [[nodiscard]] const RenderTargetStack::Node& top() const noexcept;
    [[nodiscard]] RenderTargetStack::Node& top() noexcept;
    void push(const RenderTargetStack::Node& node) noexcept;
    void push(RenderTargetStack::Node&& node) noexcept;
    void pop() noexcept;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::size_t size() const;

protected:
private:
    std::stack<RenderTargetStack::Node, std::vector<RenderTargetStack::Node>> _stack{};
};

[[nodiscard]] bool operator==(const RenderTargetStack::Node& lhs, const RenderTargetStack::Node& rhs);
[[nodiscard]] bool operator!=(const RenderTargetStack::Node& lhs, const RenderTargetStack::Node& rhs);
