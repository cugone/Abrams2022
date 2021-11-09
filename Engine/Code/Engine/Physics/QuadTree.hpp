#pragma once

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <type_traits>
#include <vector>

template<typename T>
class QuadTree {
public:
    QuadTree() = default;
    explicit QuadTree(const AABB2& bounds);
    QuadTree(const QuadTree& other) = delete;
    QuadTree(QuadTree&& other) = default;
    QuadTree& operator=(const QuadTree& other) = delete;
    QuadTree& operator=(QuadTree&& other) = default;
    ~QuadTree() = default;

    void Add(std::add_pointer_t<T> new_element);
    void Add(std::vector<std::add_pointer_t<T>> new_elements);
    void Clear();
    void DebugRender() const;

    void SetWorldBounds(const AABB2& bounds) noexcept;
    [[nodiscard]] std::vector<std::add_pointer_t<T>> Query(const AABB2& area) noexcept;

protected:
private:
    explicit QuadTree(QuadTree<T>* parent, const AABB2& bounds);
    explicit QuadTree(QuadTree<T>* parent, const AABB2& bounds, const std::vector<T>& elements);
    // clang-format off
    enum class ChildID {
        BottomLeft
        , TopLeft
        , TopRight
        , BottomRight
    };
    // clang-format off
    void DebugRender_helper() const;
    [[nodiscard]] bool IsParent(const QuadTree<T>* node) const;
    [[nodiscard]] bool IsChild(const QuadTree<T>* node) const;
    [[nodiscard]] bool IsLeaf(const QuadTree<T>* node) const;
    [[nodiscard]] bool IsParent() const;
    [[nodiscard]] bool IsChild() const;
    [[nodiscard]] bool IsLeaf() const;
    [[nodiscard]] bool IsElementIntersectingMe(std::add_pointer_t<T> new_element) const;
    [[nodiscard]] bool NeedsSubdivide() const;
    [[nodiscard]] bool NeedsUnSubdivide() const;
    void Subdivide();
    void UnSubdivide();
    void MakeChildren();
    void ClearChildren();
    void AddElement(std::add_pointer_t<T> old_element);
    void GiveElementsToChildren();
    void TakeElementsFromChildren();
    [[nodiscard]] const QuadTree<T>* GetChild(const ChildID& id) const;
    [[nodiscard]] QuadTree<T>* GetChild(const ChildID& id);
    void CreateChild(const ChildID& id);
    void DeleteChild(const ChildID& id);
    void SetChild(const ChildID& id, std::unique_ptr<QuadTree<T>> child);

    [[nodiscard]] const std::size_t ChildIdToIndex(const ChildID& id) const;
    [[nodiscard]] bool IsLeaf(const QuadTree<T>& node) const;
    [[nodiscard]] AABB2 GetBounds() const;
    [[nodiscard]] AABB2 GetParentBounds() const;
    [[nodiscard]] std::vector<QuadTree<T>*> GetNodesByElement(const T& object) const noexcept;

    QuadTree<T>* m_parent = nullptr;
    Vector2 m_half_extents = Vector2::One;
    AABB2 m_bounds = AABB2{-m_half_extents, m_half_extents};
    std::array<std::unique_ptr<QuadTree>, 4> m_children{};
    const int m_maxElementsBeforeSubdivide = 2;
    const int m_maxChildren = 4;
    const int m_maxDepth = 8;
    std::vector<std::add_pointer_t<T>> m_elements{};
};

template<typename T>
std::vector<std::add_pointer_t<T>> QuadTree<T>::Query(const AABB2& area) noexcept {
    std::vector<std::add_pointer_t<T>> result{};
    if(MathUtils::DoAABBsOverlap(area, m_bounds)) {
        if(!IsLeaf(*this)) {
            for(const auto& c : m_children) {
                if(c) {
                    auto inner_result = c->Query(area);
                    result.insert(std::end(result), std::begin(inner_result), std::end(inner_result));
                }
            }
        } else {
            for(auto* elem : m_elements) {
                if(elem) {
                    if(MathUtils::DoOBBsOverlap(OBB2(area), elem->GetBounds())) {
                        result.push_back(elem);
                    }
                }
            }
        }
    }
    return result;
}

template<typename T>
AABB2 QuadTree<T>::GetParentBounds() const {
    return m_parent ? m_parent->m_bounds : AABB2{};
}

template<typename T>
bool QuadTree<T>::IsLeaf(const QuadTree<T>* node) const {
    return node ? node->m_children[0] == nullptr : false;
}

template<typename T>
bool QuadTree<T>::IsChild(const QuadTree<T>* node) const {
    return node ? node->m_parent != nullptr : false;
}

template<typename T>
bool QuadTree<T>::IsParent(const QuadTree<T>* node) const {
    return node ? node->m_children[0] != nullptr : false;
}

template<typename T>
bool QuadTree<T>::IsLeaf() const {
    return m_children[0] == nullptr;
}

template<typename T>
bool QuadTree<T>::IsChild() const {
    return m_parent != nullptr;
}

template<typename T>
bool QuadTree<T>::IsParent() const {
    return m_children[0] != nullptr;
}

template<typename T>
void QuadTree<T>::SetWorldBounds(const AABB2& bounds) noexcept {
    m_bounds = bounds;
    m_half_extents = m_bounds.CalcDimensions() * 0.5f;
    if(IsParent()) {
        auto* bl = GetChild(ChildID::BottomLeft);
        bl->m_half_extents = m_half_extents * 0.5f;
        bl->m_bounds.mins = -m_half_extents;
        bl->m_bounds.maxs = m_half_extents;
        bl->m_bounds.Translate(GetParentBounds().CalcCenter() + Vector2{-m_half_extents.x, +m_half_extents.y});
        auto* tl = GetChild(ChildID::TopLeft);
        tl->m_half_extents = m_half_extents * 0.5f;
        tl->m_bounds.mins = -m_half_extents;
        tl->m_bounds.maxs = m_half_extents;
        tl->m_bounds.Translate(GetParentBounds().CalcCenter() + Vector2{-m_half_extents.x, -m_half_extents.y});
        auto* tr = GetChild(ChildID::TopRight);
        tr->m_half_extents = m_half_extents * 0.5f;
        tr->m_bounds.mins = -m_half_extents;
        tr->m_bounds.maxs = m_half_extents;
        tr->m_bounds.Translate(GetParentBounds().CalcCenter() + Vector2{+m_half_extents.x, -m_half_extents.y});
        auto* br = GetChild(ChildID::BottomRight);
        br->m_half_extents = m_half_extents * 0.5f;
        br->m_bounds.mins = -m_half_extents;
        br->m_bounds.maxs = m_half_extents;
        br->m_bounds.Translate(GetParentBounds().CalcCenter() + Vector2{+m_half_extents.x, +m_half_extents.y});
        for(auto& child : m_children) {
            if(child) {
                child->SetWorldBounds(child->m_bounds);
            }
        }
    }
}

template<typename T>
void QuadTree<T>::Clear() {
    for(auto& child : m_children) {
        if(child) {
            child->Clear();
        }
    }
    m_elements.clear();
    for(auto& child : m_children) {
        child.reset(nullptr);
    }
}

template<typename T>
void QuadTree<T>::Add(std::add_pointer_t<T> new_element) {
    if(!IsElementIntersectingMe(new_element)) {
        return;
    }
    PROFILE_LOG_SCOPE_FUNCTION();
    if(!IsLeaf(*this)) {
        for(auto& child : m_children) {
            if(child) {
                child->Add(new_element);
            }
        }
        return;
    }
    m_elements.push_back(new_element);
    Subdivide();
}

template<typename T>
void QuadTree<T>::Add(std::vector<std::add_pointer_t<T>> new_elements) {
    m_elements.reserve(m_elements.size() + new_elements.size());
    for(const auto& elem : new_elements) {
        Add(elem);
    }
}

template<typename T>
bool QuadTree<T>::IsLeaf(const QuadTree<T>& node) const {
    return node.m_children[0] == nullptr;
}

template<typename T>
AABB2 QuadTree<T>::GetBounds() const {
    return m_bounds;
}

template<typename T>
void QuadTree<T>::AddElement(std::add_pointer_t<T> new_element) {
    m_elements.push_back(new_element);
}

template<typename T>
void QuadTree<T>::DebugRender() const {
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.SetMaterial(renderer.GetMaterial("__2D"));
    renderer.SetModelMatrix(Matrix4::I);
    DebugRender_helper();
}

template<typename T>
const std::size_t QuadTree<T>::ChildIdToIndex(const ChildID& id) const {
    switch(id) {
    case ChildID::BottomLeft: return 0;
    case ChildID::BottomRight: return 1;
    case ChildID::TopLeft: return 2;
    case ChildID::TopRight: return 3;
    default:
        ERROR_AND_DIE("QuadTree: ChildToIndex invalid index.")
    }
}

template<typename T>
const QuadTree<T>* QuadTree<T>::GetChild(const ChildID& id) const {
    const auto index = ChildIdToIndex(id);
    return m_children[index].get();
}

template<typename T>
QuadTree<T>* QuadTree<T>::GetChild(const ChildID& id) {
    const auto index = ChildIdToIndex(id);
    return m_children[index].get();
}

template<typename T>
void QuadTree<T>::CreateChild(const ChildID& id) {
    const auto index = ChildIdToIndex(id);

    auto bounds = m_bounds;
    bounds.ScalePadding(0.50f, 0.50f);
    switch(id) {
    case ChildID::BottomLeft: {
        const auto tl_corner = m_bounds.CalcCenter() + Vector2(-m_half_extents.x, 0.0f);
        const auto pos = tl_corner + Vector2(m_half_extents.x, m_half_extents.y) * 0.5f;
        bounds.SetPosition(pos);
        m_children[index] = std::move(std::unique_ptr<QuadTree<T>>(new QuadTree<T>(this, bounds)));
        break;
    }
    case ChildID::BottomRight: {
        const auto tl_corner = m_bounds.CalcCenter();
        const auto pos = tl_corner + m_half_extents * 0.50f;
        bounds.SetPosition(pos);
        m_children[index] = std::move(std::unique_ptr<QuadTree<T>>(new QuadTree<T>(this, bounds)));
        break;
    }
    case ChildID::TopLeft: {
        //bounds defaults to TopLeft by virtue of scale padding.
        m_children[index] = std::move(std::unique_ptr<QuadTree<T>>(new QuadTree<T>(this, bounds)));
        break;
    }
    case ChildID::TopRight: {
        const auto tl_corner = m_bounds.CalcCenter() + Vector2(0.0f, -m_half_extents.y);
        const auto pos = tl_corner + m_half_extents * 0.50f;
        bounds.SetPosition(pos);
        m_children[index] = std::move(std::unique_ptr<QuadTree<T>>(new QuadTree<T>(this, bounds)));
        break;
    }
    default:
        ERROR_AND_DIE("QuadTree Child ID has changed.");
    }
}

template<typename T>
void QuadTree<T>::DeleteChild(const ChildID& id) {
    const auto index = ChildIdToIndex(id);
    m_children[index].reset(nullptr);
}

template<typename T>
void QuadTree<T>::SetChild(const ChildID& id, std::unique_ptr<QuadTree<T>> child) {
    const auto index = ChildIdToIndex(id);
    return m_children[index].get();
}

template<typename T>
QuadTree<T>::QuadTree(const AABB2& bounds)
: m_half_extents{bounds.CalcDimensions() * 0.5f}
, m_bounds{bounds} {
    /* DO NOTHING */
}

template<typename T>
QuadTree<T>::QuadTree(QuadTree<T>* parent, const AABB2& bounds)
: m_parent(parent)
, m_half_extents(bounds.CalcDimensions() * 0.5f)
, m_bounds(bounds) {
    /* DO NOTHING */
}

template<typename T>
QuadTree<T>::QuadTree(QuadTree<T>* parent, const AABB2& bounds, const std::vector<T>& elements)
: m_parent(parent)
, m_half_extents(bounds.CalcDimensions() * 0.5f)
, m_bounds(bounds)
, m_elements(elements) {
    /* DO NOTHING */
}

template<typename T>
void QuadTree<T>::Subdivide() {
    if(NeedsSubdivide()) {
        PROFILE_LOG_SCOPE("Subdivide after needs to.");
        MakeChildren();
        GiveElementsToChildren();
    }
}

template<typename T>
void QuadTree<T>::UnSubdivide() {
    if(NeedsUnSubdivide()) {
        PROFILE_LOG_SCOPE("UnSubdivide after needs to.");
        TakeElementsFromChildren();
        ClearChildren();
    }
}

template<typename T>
void QuadTree<T>::MakeChildren() {
    CreateChild(ChildID::BottomLeft);
    CreateChild(ChildID::TopLeft);
    CreateChild(ChildID::TopRight);
    CreateChild(ChildID::BottomRight);
}

template<typename T>
void QuadTree<T>::ClearChildren() {
    DeleteChild(ChildID::BottomLeft);
    DeleteChild(ChildID::TopLeft);
    DeleteChild(ChildID::TopRight);
    DeleteChild(ChildID::BottomRight);
}

template<typename T>
void QuadTree<T>::GiveElementsToChildren() {
    PROFILE_LOG_SCOPE_FUNCTION();
    for(auto& elem : m_elements) {
        for(auto& child : m_children) {
            if(child) {
                if(MathUtils::Contains(child->GetBounds(), elem->GetBounds())) {
                    child->AddElement(elem);
                    elem = nullptr;
                    break;
                }
            }
        }
    }
    m_elements.erase(std::remove_if(std::begin(m_elements), std::end(m_elements), [](const T* a) { return a == nullptr; }), std::end(m_elements));
    for(auto& child : m_children) {
        if(child) {
            child->Subdivide();
        }
    }
}

template<typename T>
void QuadTree<T>::TakeElementsFromChildren() {
    PROFILE_LOG_SCOPE_FUNCTION();
    const auto max_elems = std::size_t{0};
    for(auto& child : m_children) {
        max_elems += child->m_elements.size();
    }
    m_elements.reserve(max_elems);
    for(auto& child : m_children) {
        std::merge(std::begin(child->m_elements), std::end(child->m_elements), std::begin(m_elements), std::end(m_elements), std::back_inserter(m_elements));
    }
}

template<typename T>
void QuadTree<T>::DebugRender_helper() const {
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.DrawAABB2(m_bounds, Rgba::Green, Rgba::NoAlpha);
    for(const auto& child : m_children) {
        if(child) {
            child->DebugRender_helper();
        }
    }
}

template<typename T>
bool QuadTree<T>::NeedsSubdivide() const {
    return !((std::min)(m_half_extents.x, m_half_extents.y) < 0.5) && m_maxElementsBeforeSubdivide < m_elements.size();
}

template<typename T>
bool QuadTree<T>::NeedsUnSubdivide() const {
    return !(m_maxElementsBeforeSubdivide < m_elements.size());
}

template<typename T>
bool QuadTree<T>::IsElementIntersectingMe(std::add_pointer_t<T> new_element) const {
    if(new_element) {
        return MathUtils::DoOBBsOverlap(OBB2(GetBounds()), OBB2(new_element->GetBounds()));
    }
    return false;
}

template<typename T>
std::vector<QuadTree<T>*> QuadTree<T>::GetNodesByElement(const T& object) const noexcept {
    std::vector<QuadTree<T>*> result{};
    if(std::end(m_elements) != std::find(std::begin(m_elements), std::end(m_elements), &object)) {
        result.push_back(this);
    }
    if(IsParent()) {
        for(const auto* c : m_children) {
            const auto& child_result = c->GetNodesByElement(object);
            for(const auto* c : child_result) {
                result.push_back(c);
            }
        }
    }
    return result;
}
