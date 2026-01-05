#pragma once

#include "Panel.hpp"
#include "core/Types.hpp"
#include <entt/entt.hpp>

namespace Editor {

class SceneHierarchyPanel : public Panel {
public:
    SceneHierarchyPanel() : Panel("Scene Hierarchy") {}

    void OnImGuiRender() override;

private:
    void DrawEntityNode(entt::entity entity);
    void DrawEntityContextMenu(entt::entity entity);

    Engine::String GetEntityDisplayName(entt::entity entity);

    // Rename state
    entt::entity m_RenamingEntity = entt::null;
    Engine::String m_RenameBuffer;
};

} // namespace Editor
