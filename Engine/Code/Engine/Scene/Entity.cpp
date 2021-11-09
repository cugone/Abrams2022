#include "Engine/Scene/Entity.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Scene/Scene.hpp"

Entity::Entity(std::uint32_t handle, std::weak_ptr<Scene> scene) noexcept
    : m_id(static_cast<entt::entity>(handle))
{
    GUARANTEE_OR_DIE(!scene.expired(), "Scene reference has expired.");
    m_Scene = scene;
}

Scene* Entity::GetScene() const noexcept {
    if(auto scene = m_Scene.lock(); scene) {
        return scene.get();
    }
    return nullptr;
}

Scene* Entity::GetScene() noexcept {
    if(auto scene = m_Scene.lock(); scene) {
        return scene.get();
    }
    return nullptr;
}

bool Entity::HasParent() const noexcept {
    return m_parent != nullptr;
}

Entity* Entity::GetParent() const noexcept {
    return m_parent;
}

Entity Entity::AddChild(Entity&& entity) noexcept {
    m_children.emplace_back(std::move(entity));
    return {m_children.back()};
}

Entity Entity::AddChild(const Entity& entity) noexcept {
    m_children.emplace_back(entity);
    return Entity{m_children.back()};
}

bool Entity::HasChildren() const noexcept {
    return m_children.empty();
}

const std::vector<Entity>& Entity::GetChildren() const noexcept {
    return m_children;
}

std::vector<Entity>& Entity::GetChildren() noexcept {
    return m_children;
}

bool Entity::HasComponents() const noexcept {
    GUARANTEE_OR_DIE(!m_Scene.expired(), "Scene reference has expired.");
    return m_Scene.lock()->m_registry.orphan(m_id) == false;
}
