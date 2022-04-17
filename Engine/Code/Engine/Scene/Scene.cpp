#include "Engine/Scene/Scene.hpp"

#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Entity.hpp"

#include <memory>

Scene::~Scene() noexcept {
    m_registry.clear();
}

a2de::Entity Scene::CreateEntity() noexcept {
    return CreateEntity("Entity");
}

a2de::Entity Scene::CreateEntity(const std::string& name) noexcept {
    return CreateEntityWithUUID(a2de::UUID(), name);
}

a2de::Entity Scene::CreateEntityWithUUID(a2de::UUID uuid, const std::string& name) noexcept {
    auto entity = a2de::Entity(static_cast<std::uint32_t>(m_registry.create()), get());
    entity.AddComponent<IdComponent>(uuid);
    entity.AddComponent<TagComponent>(name);
    return entity;
}

void Scene::DestroyEntity(a2de::Entity e) noexcept {
    m_registry.destroy(e.m_id);
}

std::weak_ptr<const Scene> Scene::get() const noexcept {
    return weak_from_this();
}

std::weak_ptr<Scene> Scene::get() noexcept {
    return weak_from_this();
}

const entt::registry& Scene::GetRegistry() const noexcept {
    return m_registry;
}

entt::registry& Scene::GetRegistry() noexcept {
    return m_registry;
}
