#include "SceneSnapshot.hpp"
#include "ecs/Components/Transform.hpp"
#include "ecs/Components/Renderable.hpp"
#include "ecs/Components/Hierarchy.hpp"
#include "ecs/Components/LightComponents.hpp"

namespace Editor {

void SceneSnapshot::Capture(Engine::Registry& registry) {
    m_SnapshotRegistry.clear();

    auto& srcRegistry = registry.Raw();

    // Clone all entities and their components
    // Use view to iterate all entities with Transform (all entities should have Transform)
    auto view = srcRegistry.view<Engine::Transform>();
    for (auto entity : view) {
        auto newEntity = m_SnapshotRegistry.create(entity);

        // Copy Transform
        if (auto* comp = srcRegistry.try_get<Engine::Transform>(entity)) {
            m_SnapshotRegistry.emplace<Engine::Transform>(newEntity, *comp);
        }

        // Copy MeshComponent
        if (auto* comp = srcRegistry.try_get<Engine::MeshComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::MeshComponent>(newEntity, *comp);
        }

        // Copy MaterialComponent
        if (auto* comp = srcRegistry.try_get<Engine::MaterialComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::MaterialComponent>(newEntity, *comp);
        }

        // Copy Renderable
        if (auto* comp = srcRegistry.try_get<Engine::Renderable>(entity)) {
            m_SnapshotRegistry.emplace<Engine::Renderable>(newEntity, *comp);
        }

        // Copy Hierarchy
        if (auto* comp = srcRegistry.try_get<Engine::Hierarchy>(entity)) {
            m_SnapshotRegistry.emplace<Engine::Hierarchy>(newEntity, *comp);
        }

        // Copy DirectionalLightComponent
        if (auto* comp = srcRegistry.try_get<Engine::DirectionalLightComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::DirectionalLightComponent>(newEntity, *comp);
        }

        // Copy PointLightComponent
        if (auto* comp = srcRegistry.try_get<Engine::PointLightComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::PointLightComponent>(newEntity, *comp);
        }

        // Copy SpotLightComponent
        if (auto* comp = srcRegistry.try_get<Engine::SpotLightComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::SpotLightComponent>(newEntity, *comp);
        }

        // Copy AmbientLightComponent
        if (auto* comp = srcRegistry.try_get<Engine::AmbientLightComponent>(entity)) {
            m_SnapshotRegistry.emplace<Engine::AmbientLightComponent>(newEntity, *comp);
        }
    }

    m_HasSnapshot = true;
}

void SceneSnapshot::Restore(Engine::Registry& registry) {
    if (!m_HasSnapshot) return;

    auto& dstRegistry = registry.Raw();
    dstRegistry.clear();

    // Restore from snapshot - iterate over all entities with Transform
    auto view = m_SnapshotRegistry.view<Engine::Transform>();
    for (auto entity : view) {
        auto newEntity = dstRegistry.create(entity);

        // Restore Transform
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::Transform>(entity)) {
            dstRegistry.emplace<Engine::Transform>(newEntity, *comp);
        }

        // Restore MeshComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::MeshComponent>(entity)) {
            dstRegistry.emplace<Engine::MeshComponent>(newEntity, *comp);
        }

        // Restore MaterialComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::MaterialComponent>(entity)) {
            dstRegistry.emplace<Engine::MaterialComponent>(newEntity, *comp);
        }

        // Restore Renderable
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::Renderable>(entity)) {
            dstRegistry.emplace<Engine::Renderable>(newEntity, *comp);
        }

        // Restore Hierarchy
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::Hierarchy>(entity)) {
            dstRegistry.emplace<Engine::Hierarchy>(newEntity, *comp);
        }

        // Restore DirectionalLightComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::DirectionalLightComponent>(entity)) {
            dstRegistry.emplace<Engine::DirectionalLightComponent>(newEntity, *comp);
        }

        // Restore PointLightComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::PointLightComponent>(entity)) {
            dstRegistry.emplace<Engine::PointLightComponent>(newEntity, *comp);
        }

        // Restore SpotLightComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::SpotLightComponent>(entity)) {
            dstRegistry.emplace<Engine::SpotLightComponent>(newEntity, *comp);
        }

        // Restore AmbientLightComponent
        if (auto* comp = m_SnapshotRegistry.try_get<Engine::AmbientLightComponent>(entity)) {
            dstRegistry.emplace<Engine::AmbientLightComponent>(newEntity, *comp);
        }
    }
}

void SceneSnapshot::Clear() {
    m_SnapshotRegistry.clear();
    m_HasSnapshot = false;
}

} // namespace Editor
