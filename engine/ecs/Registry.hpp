#pragma once

#include <entt/entt.hpp>
#include "core/Types.hpp"
#include "Component.hpp"
#include <functional>

namespace Engine {

// Entity handle wrapper for cleaner API
class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, entt::registry* registry)
        : m_Handle(handle), m_Registry(registry) {}

    // Check if entity is valid
    bool IsValid() const {
        return m_Registry && m_Registry->valid(m_Handle);
    }

    operator bool() const { return IsValid(); }
    operator entt::entity() const { return m_Handle; }

    // Component operations
    template<typename T, typename... Args>
    T& AddComponent(Args&&... args) {
        return m_Registry->emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T& AddOrReplaceComponent(Args&&... args) {
        return m_Registry->emplace_or_replace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template<typename T>
    T& GetComponent() {
        return m_Registry->get<T>(m_Handle);
    }

    template<typename T>
    const T& GetComponent() const {
        return m_Registry->get<T>(m_Handle);
    }

    template<typename T>
    T* TryGetComponent() {
        return m_Registry->try_get<T>(m_Handle);
    }

    template<typename T>
    const T* TryGetComponent() const {
        return m_Registry->try_get<T>(m_Handle);
    }

    template<typename T>
    bool HasComponent() const {
        return m_Registry->all_of<T>(m_Handle);
    }

    template<typename... T>
    bool HasAllComponents() const {
        return m_Registry->all_of<T...>(m_Handle);
    }

    template<typename... T>
    bool HasAnyComponent() const {
        return m_Registry->any_of<T...>(m_Handle);
    }

    template<typename T>
    void RemoveComponent() {
        m_Registry->remove<T>(m_Handle);
    }

    // Get raw handle
    entt::entity GetHandle() const { return m_Handle; }
    entt::registry* GetRegistry() { return m_Registry; }

    // Comparison operators
    bool operator==(const Entity& other) const {
        return m_Handle == other.m_Handle && m_Registry == other.m_Registry;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

private:
    entt::entity m_Handle{entt::null};
    entt::registry* m_Registry = nullptr;
};

// Registry wrapper with additional functionality
class Registry {
public:
    Registry() {
        // Initialize component metadata
        ComponentRegistry::Get().InitializeMeta(m_MetaContext);
    }

    ~Registry() = default;

    // Entity creation/destruction
    Entity CreateEntity() {
        return Entity(m_Registry.create(), &m_Registry);
    }

    Entity CreateEntity(const String& name) {
        auto entity = CreateEntity();
        // You could add a NameComponent here if needed
        (void)name;
        return entity;
    }

    void DestroyEntity(Entity entity) {
        if (entity.IsValid()) {
            m_Registry.destroy(entity.GetHandle());
        }
    }

    void DestroyEntity(entt::entity entity) {
        if (m_Registry.valid(entity)) {
            m_Registry.destroy(entity);
        }
    }

    // Check if entity is valid
    bool IsValid(entt::entity entity) const {
        return m_Registry.valid(entity);
    }

    // Get entity wrapper from handle
    Entity GetEntity(entt::entity handle) {
        return Entity(handle, &m_Registry);
    }

    // Direct access to underlying registry
    entt::registry& Raw() { return m_Registry; }
    const entt::registry& Raw() const { return m_Registry; }

    // View creation
    template<typename... Components>
    auto View() {
        return m_Registry.view<Components...>();
    }

    template<typename... Components>
    auto View() const {
        return m_Registry.view<Components...>();
    }

    // Group creation (more efficient for frequently used component combinations)
    template<typename... Owned, typename... Get, typename... Exclude>
    auto Group(entt::get_t<Get...> = {}, entt::exclude_t<Exclude...> = {}) {
        return m_Registry.group<Owned...>(entt::get<Get...>, entt::exclude<Exclude...>);
    }

    // Entity count
    usize EntityCount() const {
        auto* storage = m_Registry.storage<entt::entity>();
        return storage ? storage->size() - storage->free_list() : 0;
    }

    // Component count
    template<typename T>
    usize ComponentCount() const {
        auto* storage = m_Registry.storage<T>();
        return storage ? storage->size() : 0;
    }

    // Iterate over all entities with specific components
    template<typename... Components, typename Func>
    void Each(Func&& func) {
        auto view = m_Registry.view<Components...>();
        view.each(std::forward<Func>(func));
    }

    // Clear all entities and components
    void Clear() {
        m_Registry.clear();
    }

    // Sort components by a specific criteria
    template<typename T, typename Compare>
    void Sort(Compare compare) {
        m_Registry.sort<T>(compare);
    }

    // Runtime component operations (for editor/scripting)
    void* AddComponentByName(entt::entity entity, const char* componentName) {
        auto* factory = ComponentRegistry::Get().GetFactoryByName(componentName);
        if (factory && factory->Create) {
            return factory->Create(m_Registry, entity);
        }
        return nullptr;
    }

    void RemoveComponentByName(entt::entity entity, const char* componentName) {
        auto* factory = ComponentRegistry::Get().GetFactoryByName(componentName);
        if (factory && factory->Remove) {
            factory->Remove(m_Registry, entity);
        }
    }

    bool HasComponentByName(entt::entity entity, const char* componentName) const {
        auto* factory = ComponentRegistry::Get().GetFactoryByName(componentName);
        if (factory && factory->Has) {
            return factory->Has(m_Registry, entity);
        }
        return false;
    }

    // Get meta context for reflection
    entt::meta_ctx& GetMetaContext() { return m_MetaContext; }

    // Signals/observers for component changes
    template<typename T>
    auto OnConstruct() {
        return m_Registry.on_construct<T>();
    }

    template<typename T>
    auto OnDestroy() {
        return m_Registry.on_destroy<T>();
    }

    template<typename T>
    auto OnUpdate() {
        return m_Registry.on_update<T>();
    }

private:
    entt::registry m_Registry;
    entt::meta_ctx m_MetaContext;
};

} // namespace Engine
