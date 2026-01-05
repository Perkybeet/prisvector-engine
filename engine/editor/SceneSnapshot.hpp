#pragma once

#include "ecs/Registry.hpp"
#include <entt/entt.hpp>

namespace Editor {

// Captures and restores scene state for Play mode
class SceneSnapshot {
public:
    // Take a snapshot of the current scene state
    void Capture(Engine::Registry& registry);

    // Restore the scene to the captured state
    void Restore(Engine::Registry& registry);

    // Check if snapshot exists
    bool HasSnapshot() const { return m_HasSnapshot; }

    // Clear the snapshot
    void Clear();

private:
    entt::registry m_SnapshotRegistry;
    bool m_HasSnapshot = false;
};

} // namespace Editor
