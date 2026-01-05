#pragma once

// =============================================================================
// ECS Core - Main include file for Entity Component System
// =============================================================================
//
// Usage:
//   #include "ecs/Core.hpp"
//
// This provides access to:
//   - Entity, Registry (entity management)
//   - ISystem, SystemScheduler (system management)
//   - Component reflection macros (REFLECT_COMPONENT, REFLECT_TAG)
//   - Built-in components (Transform, Hierarchy, Renderable)
//
// Example:
//   Registry registry;
//   Entity entity = registry.CreateEntity();
//   entity.AddComponent<Transform>();
//   entity.GetComponent<Transform>().SetPosition(1.0f, 2.0f, 3.0f);
//
// =============================================================================

// Core ECS types
#include "ecs/Component.hpp"
#include "ecs/Registry.hpp"
#include "ecs/System.hpp"
#include "ecs/SystemScheduler.hpp"

// Built-in components
#include "ecs/Components/Transform.hpp"
#include "ecs/Components/Hierarchy.hpp"
#include "ecs/Components/Renderable.hpp"

namespace Engine {

// =============================================================================
// Common component combinations as type aliases
// =============================================================================

// Entities that can be rendered
template<typename... Extra>
using RenderableQuery = std::tuple<Transform, Renderable, Extra...>;

// Entities with hierarchy (scene graph)
template<typename... Extra>
using HierarchyQuery = std::tuple<Transform, Hierarchy, Extra...>;

// =============================================================================
// System utilities
// =============================================================================

// TransformSystem - updates world matrices respecting hierarchy
class TransformSystem : public ISystem {
public:
    DEFINE_SYSTEM(TransformSystem, Update, 10)

    void OnUpdate(entt::registry& registry, f32 deltaTime) override {
        (void)deltaTime;

        // First pass: update root transforms
        auto rootView = registry.view<Transform, RootEntity>();
        for (auto entity : rootView) {
            auto& transform = registry.get<Transform>(entity);
            if (transform.Dirty) {
                transform.UpdateWorldMatrix();
            }
        }

        // Second pass: update transforms with hierarchy (sorted by depth)
        auto hierarchyView = registry.view<Transform, Hierarchy>();

        // Collect and sort by depth
        Vector<std::pair<u32, entt::entity>> sortedEntities;
        for (auto entity : hierarchyView) {
            auto& hierarchy = registry.get<Hierarchy>(entity);
            if (!registry.all_of<RootEntity>(entity)) {
                sortedEntities.emplace_back(hierarchy.Depth, entity);
            }
        }

        std::sort(sortedEntities.begin(), sortedEntities.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        // Update in depth order
        for (auto& [depth, entity] : sortedEntities) {
            auto& transform = registry.get<Transform>(entity);
            auto& hierarchy = registry.get<Hierarchy>(entity);

            if (transform.Dirty || hierarchy.Parent != entt::null) {
                glm::mat4 parentMatrix{1.0f};
                if (hierarchy.Parent != entt::null) {
                    auto* parentTransform = registry.try_get<Transform>(hierarchy.Parent);
                    if (parentTransform) {
                        parentMatrix = parentTransform->WorldMatrix;
                    }
                }
                transform.UpdateWorldMatrix(parentMatrix);
            }
        }
    }
};

// BoundsUpdateSystem - updates world bounds for renderables
class BoundsUpdateSystem : public ISystem {
public:
    DEFINE_SYSTEM(BoundsUpdateSystem, PreRender, 5)

    void OnUpdate(entt::registry& registry, f32 deltaTime) override {
        (void)deltaTime;

        auto view = registry.view<Transform, Renderable, MeshComponent>();
        for (auto entity : view) {
            auto& transform = registry.get<Transform>(entity);
            auto& renderable = registry.get<Renderable>(entity);
            auto& mesh = registry.get<MeshComponent>(entity);

            // Transform local bounds to world bounds
            renderable.WorldBounds = mesh.LocalBounds.Transform(transform.WorldMatrix);
            renderable.WorldSphere = BoundingSphere::FromAABB(renderable.WorldBounds);
        }
    }
};

} // namespace Engine
