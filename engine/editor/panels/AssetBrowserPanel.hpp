#pragma once

#include "Panel.hpp"
#include "renderer/Mesh.hpp"

namespace Editor {

class AssetBrowserPanel : public Panel {
public:
    AssetBrowserPanel(Engine::Ref<Engine::Mesh> cube,
                      Engine::Ref<Engine::Mesh> sphere,
                      Engine::Ref<Engine::Mesh> plane,
                      Engine::Ref<Engine::Mesh> cylinder);

    void OnImGuiRender() override;

private:
    void DrawMeshesTab();
    void DrawShadersTab();
    void DrawTexturesTab();

    Engine::Ref<Engine::Mesh> m_CubeMesh;
    Engine::Ref<Engine::Mesh> m_SphereMesh;
    Engine::Ref<Engine::Mesh> m_PlaneMesh;
    Engine::Ref<Engine::Mesh> m_CylinderMesh;
};

} // namespace Editor
