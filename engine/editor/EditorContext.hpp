#pragma once

#include "core/Types.hpp"
#include "ecs/Registry.hpp"
#include "camera/CameraManager.hpp"
#include "renderer/lighting/DeferredLightingSystem.hpp"
#include "renderer/shadows/ShadowMapSystem.hpp"
#include "renderer/debug/DebugRenderer.hpp"
#include <glm/glm.hpp>
#include <entt/entt.hpp>

namespace Editor {

enum class GizmoMode {
    Translate,
    Rotate,
    Scale
};

enum class GizmoSpace {
    Local,
    World
};

enum class PlayState {
    Edit,
    Play,
    Pause
};

struct EditorContext {
    // Selection
    entt::entity SelectedEntity = entt::null;
    Engine::Vector<entt::entity> MultiSelection;

    // Gizmo state
    GizmoMode CurrentGizmoMode = GizmoMode::Translate;
    GizmoSpace CurrentGizmoSpace = GizmoSpace::World;
    bool GizmoSnapping = false;
    glm::vec3 TranslateSnapValue{1.0f};
    float RotateSnapValue = 15.0f;
    float ScaleSnapValue = 0.5f;

    // Play state
    PlayState State = PlayState::Edit;

    // Viewport state
    bool ViewportFocused = false;
    bool ViewportHovered = false;
    glm::vec2 ViewportSize{1280.0f, 720.0f};
    glm::vec2 ViewportBounds[2];

    // Debug view
    Engine::DebugView CurrentDebugView = Engine::DebugView::None;

    // Rendering settings
    float Exposure = 1.0f;
    float Gamma = 2.2f;
    bool ShadowsEnabled = true;
    bool VSyncEnabled = true;
    bool WireframeMode = false;

    // References to engine systems (set by Editor)
    Engine::Registry* Registry = nullptr;
    Engine::CameraManager* CameraManager = nullptr;
    Engine::DeferredLightingSystem* LightingSystem = nullptr;
    Engine::ShadowMapSystem* ShadowSystem = nullptr;
    Engine::DebugRenderer* DebugRenderer = nullptr;

    // Helper methods
    bool HasSelection() const { return SelectedEntity != entt::null; }

    void ClearSelection() {
        SelectedEntity = entt::null;
        MultiSelection.clear();
    }

    void Select(entt::entity entity) {
        SelectedEntity = entity;
        MultiSelection.clear();
        MultiSelection.push_back(entity);
    }

    void AddToSelection(entt::entity entity) {
        if (std::find(MultiSelection.begin(), MultiSelection.end(), entity) == MultiSelection.end()) {
            MultiSelection.push_back(entity);
        }
        if (SelectedEntity == entt::null) {
            SelectedEntity = entity;
        }
    }

    bool IsSelected(entt::entity entity) const {
        return std::find(MultiSelection.begin(), MultiSelection.end(), entity) != MultiSelection.end();
    }
};

} // namespace Editor
