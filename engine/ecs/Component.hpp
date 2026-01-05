#pragma once

#include <entt/entt.hpp>
#include "core/Types.hpp"
#include <functional>
#include <typeindex>
#include <cstring>

namespace Engine {

// Forward declarations
class ComponentRegistry;

// Component metadata template - specialized by REFLECT_COMPONENT macro
template<typename T>
struct ComponentMeta {
    static constexpr const char* Name = "Unknown";
    static constexpr bool IsTag = false;
    static void Register(entt::meta_ctx& ctx) { (void)ctx; }
};

// Component factory for runtime component creation
struct ComponentFactory {
    const char* Name = nullptr;
    bool IsTag = false;
    std::function<void*(entt::registry&, entt::entity)> Create;
    std::function<void(entt::registry&, entt::entity)> Remove;
    std::function<bool(const entt::registry&, entt::entity)> Has;
    std::function<void*(entt::registry&, entt::entity)> Get;
    std::function<void(entt::meta_ctx&)> RegisterMeta;
    std::type_index TypeIndex = typeid(void);
};

// Global component registry - singleton
class ComponentRegistry {
public:
    static ComponentRegistry& Get() {
        static ComponentRegistry instance;
        return instance;
    }

    template<typename T>
    void Register() {
        auto typeId = entt::type_hash<T>::value();

        if (m_Factories.find(typeId) != m_Factories.end()) {
            return; // Already registered
        }

        ComponentFactory factory;
        factory.Name = ComponentMeta<T>::Name;
        factory.IsTag = ComponentMeta<T>::IsTag;
        factory.TypeIndex = std::type_index(typeid(T));

        factory.Create = [](entt::registry& reg, entt::entity e) -> void* {
            if constexpr (ComponentMeta<T>::IsTag) {
                reg.emplace<T>(e);
                return nullptr;
            } else {
                return &reg.emplace<T>(e);
            }
        };

        factory.Remove = [](entt::registry& reg, entt::entity e) {
            reg.remove<T>(e);
        };

        factory.Has = [](const entt::registry& reg, entt::entity e) -> bool {
            return reg.all_of<T>(e);
        };

        factory.Get = [](entt::registry& reg, entt::entity e) -> void* {
            if constexpr (ComponentMeta<T>::IsTag) {
                return nullptr;
            } else {
                if (reg.all_of<T>(e)) {
                    return &reg.get<T>(e);
                }
                return nullptr;
            }
        };

        factory.RegisterMeta = ComponentMeta<T>::Register;

        m_Factories[typeId] = factory;
    }

    const ComponentFactory* GetFactory(u32 typeId) const {
        auto it = m_Factories.find(typeId);
        return it != m_Factories.end() ? &it->second : nullptr;
    }

    const ComponentFactory* GetFactoryByName(const char* name) const {
        for (const auto& [id, factory] : m_Factories) {
            if (std::strcmp(factory.Name, name) == 0) {
                return &factory;
            }
        }
        return nullptr;
    }

    const HashMap<u32, ComponentFactory>& GetAllFactories() const {
        return m_Factories;
    }

    // Initialize all registered component metadata
    void InitializeMeta(entt::meta_ctx& ctx) {
        for (auto& [id, factory] : m_Factories) {
            if (factory.RegisterMeta) {
                factory.RegisterMeta(ctx);
            }
        }
    }

private:
    ComponentRegistry() = default;
    HashMap<u32, ComponentFactory> m_Factories;
};

// Helper to auto-register components at static initialization
template<typename T>
struct ComponentRegistrar {
    ComponentRegistrar() {
        ComponentRegistry::Get().Register<T>();
    }
};

} // namespace Engine

// =============================================================================
// REFLECTION MACROS
// =============================================================================

// Helper to create unique static variable names
#define REFLECT_CONCAT_IMPL(x, y) x##y
#define REFLECT_CONCAT(x, y) REFLECT_CONCAT_IMPL(x, y)
#define REFLECT_UNIQUE_NAME(base) REFLECT_CONCAT(base, __COUNTER__)

// Main macro to reflect a component with its properties
// Usage: REFLECT_COMPONENT(MyComponent,
//            .data<&MyComponent::Position>("Position"_hs)
//            .data<&MyComponent::Rotation>("Rotation"_hs))
#define REFLECT_COMPONENT(Type, ...) \
    template<> \
    struct Engine::ComponentMeta<Type> { \
        static constexpr const char* Name = #Type; \
        static constexpr bool IsTag = false; \
        static void Register(entt::meta_ctx& ctx) { \
            using namespace entt::literals; \
            entt::meta<Type>(ctx) \
                .type(entt::hashed_string{#Type}) \
                __VA_ARGS__; \
        } \
    }; \
    inline static const Engine::ComponentRegistrar<Type> REFLECT_UNIQUE_NAME(_registrar_){}

// Macro for tag components (no data)
#define REFLECT_TAG(Type) \
    template<> \
    struct Engine::ComponentMeta<Type> { \
        static constexpr const char* Name = #Type; \
        static constexpr bool IsTag = true; \
        static void Register(entt::meta_ctx& ctx) { \
            using namespace entt::literals; \
            entt::meta<Type>(ctx).type(entt::hashed_string{#Type}); \
        } \
    }; \
    inline static const Engine::ComponentRegistrar<Type> REFLECT_UNIQUE_NAME(_registrar_){}

// Simple component without reflection metadata (just registration)
#define REGISTER_COMPONENT(Type) \
    template<> \
    struct Engine::ComponentMeta<Type> { \
        static constexpr const char* Name = #Type; \
        static constexpr bool IsTag = false; \
        static void Register(entt::meta_ctx&) {} \
    }; \
    inline static const Engine::ComponentRegistrar<Type> REFLECT_UNIQUE_NAME(_registrar_){}
