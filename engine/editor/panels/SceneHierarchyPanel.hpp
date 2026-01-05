#pragma once

#include "Panel.hpp"

namespace Editor {

class SceneHierarchyPanel : public Panel {
public:
    SceneHierarchyPanel() : Panel("Scene Hierarchy") {}

    void OnImGuiRender() override;

private:
    void DrawEntityNode(entt::entity entity);
    void DrawEntityContextMenu(entt::entity entity);

    Engine::String GetEntityDisplayName(entt::entity entity);
};

} // namespace Editor
