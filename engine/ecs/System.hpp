#pragma once

#include <entt/entt.hpp>
#include "core/Types.hpp"

namespace Engine {

// Execution phases for systems - determines order of execution
enum class SystemPhase : u8 {
    PreUpdate = 0,    // Input processing, external events
    Update,           // Main game logic
    PostUpdate,       // Cleanup, preparation for physics
    PrePhysics,       // Physics preparation
    Physics,          // Physics simulation
    PostPhysics,      // Physics response
    PreRender,        // Culling, LOD calculations
    Render,           // Actual rendering
    PostRender,       // UI, debug rendering
    Count             // Number of phases
};

// Convert phase to string for debugging
inline const char* SystemPhaseToString(SystemPhase phase) {
    switch (phase) {
        case SystemPhase::PreUpdate:   return "PreUpdate";
        case SystemPhase::Update:      return "Update";
        case SystemPhase::PostUpdate:  return "PostUpdate";
        case SystemPhase::PrePhysics:  return "PrePhysics";
        case SystemPhase::Physics:     return "Physics";
        case SystemPhase::PostPhysics: return "PostPhysics";
        case SystemPhase::PreRender:   return "PreRender";
        case SystemPhase::Render:      return "Render";
        case SystemPhase::PostRender:  return "PostRender";
        default:                       return "Unknown";
    }
}

// Base interface for all systems
class ISystem {
public:
    virtual ~ISystem() = default;

    // Lifecycle methods
    virtual void OnCreate(entt::registry& registry) { (void)registry; }
    virtual void OnDestroy(entt::registry& registry) { (void)registry; }

    // Main update method - called every frame
    virtual void OnUpdate(entt::registry& registry, f32 deltaTime) = 0;

    // System identification
    virtual const char* GetName() const = 0;
    virtual SystemPhase GetPhase() const = 0;

    // Priority within phase (lower = executed first)
    virtual u32 GetPriority() const { return 100; }

    // Hot-reload callback
    virtual void OnReload() {}

    // Enable/disable system
    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }

    // Profiling
    f32 GetLastExecutionTime() const { return m_LastExecutionTime; }
    void SetLastExecutionTime(f32 time) { m_LastExecutionTime = time; }

protected:
    bool m_Enabled = true;
    f32 m_LastExecutionTime = 0.0f;
};

// Helper macro to define system metadata quickly
#define DEFINE_SYSTEM(SystemName, Phase, Priority) \
    const char* GetName() const override { return #SystemName; } \
    SystemPhase GetPhase() const override { return SystemPhase::Phase; } \
    u32 GetPriority() const override { return Priority; }

// Template base for systems that need specific component access
template<typename... Components>
class System : public ISystem {
public:
    using ComponentTuple = std::tuple<Components...>;

protected:
    // Convenience method to get view of required components
    auto GetView(entt::registry& registry) {
        return registry.view<Components...>();
    }

    // Parallel iteration helper
    template<typename Func>
    void ParallelEach(entt::registry& registry, Func&& func) {
        auto view = GetView(registry);
        // Note: For true parallelism, integrate with JobSystem
        view.each(std::forward<Func>(func));
    }
};

} // namespace Engine
