#pragma once

#include "Engine/Core/UUID.hpp"

#include "Engine/Scene/ECS.hpp"

#include <memory>
#include <string>

namespace a2de {
class Entity;
}

class Scene : public std::enable_shared_from_this<Scene> {
public:
    Scene() = default;
    Scene(const Scene& other) = default;
    Scene(Scene&& other) = default;
    Scene& operator=(const Scene& other) = default;
    Scene& operator=(Scene&& other) = default;
    virtual ~Scene() noexcept;

    a2de::Entity CreateEntity() noexcept;
    a2de::Entity CreateEntity(const std::string& name) noexcept;
    a2de::Entity CreateEntityWithUUID(a2de::UUID uuid, const std::string& name) noexcept;
    void DestroyEntity(a2de::Entity e) noexcept;

    std::weak_ptr<const Scene> get() const noexcept;
    std::weak_ptr<Scene> get() noexcept;

    const entt::registry& GetRegistry() const noexcept;
    entt::registry& GetRegistry() noexcept;

    template<typename Component>
    decltype(auto) GetEntitiesWithComponent() const noexcept {
        return m_registry.view<Component>();
    }
    
    template<typename Component>
    decltype(auto) GetEntitiesWithComponent() noexcept {
        return m_registry.view<Component>();
    }
    
    template<typename... Components>
    decltype(auto) GetEntitiesWithComponents() const noexcept {
        return m_registry.view<Components...>();
    }
    
    template<typename... Components>
    decltype(auto) GetEntitiesWithComponents() noexcept {
        return m_registry.view<Components...>();
    }

protected:
private:

    entt::registry m_registry{};
    
    friend class a2de::Entity;

};
