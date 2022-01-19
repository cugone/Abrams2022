#include "Engine/Scene/Entity.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Scene/Scene.hpp"

Entity::Entity(std::uint32_t handle, std::weak_ptr<Scene> scene) noexcept
    : m_id(static_cast<entt::entity>(handle))
{
    GUARANTEE_OR_DIE(!scene.expired(), "Scene reference has expired.");
    m_Scene = scene;
}

Entity::operator bool() const noexcept {
    return m_id != entt::null;
}

Entity::operator std::uint32_t() const noexcept {
    return static_cast<std::uint32_t>(m_id);
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
    return m_parent != nullptr && *m_parent;
}

Entity* Entity::GetParent() const noexcept {
    return m_parent;
}

void Entity::SetParent(Entity* newParent) noexcept {
    if(!newParent && m_parent) {
        m_parent->RemoveChild(*this);
    }
    m_parent = newParent;
    if(m_parent) {
        m_parent->AddChild(*this);
    }
}

Entity Entity::AddChild(Entity&& entity) noexcept {
    m_children.emplace_back(std::move(entity));
    return Entity{m_children.back()};
}

Entity Entity::AddChild(const Entity& entity) noexcept {
    m_children.emplace_back(entity);
    return Entity{m_children.back()};
}

void Entity::RemoveChild(Entity&& entity) noexcept {
    m_children.erase(std::remove(std::begin(m_children), std::end(m_children), entity), std::end(m_children));
}

void Entity::RemoveChild(const Entity& entity) noexcept {
    m_children.erase(std::remove(std::begin(m_children), std::end(m_children), entity), std::end(m_children));
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
