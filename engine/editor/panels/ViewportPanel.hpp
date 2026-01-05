#pragma once

#include "Panel.hpp"
#include "../EditorCamera.hpp"
#include "renderer/pipeline/Framebuffer.hpp"
#include "renderer/lighting/DeferredLightingSystem.hpp"
#include "renderer/shadows/ShadowMapSystem.hpp"
#include "renderer/debug/DebugRenderer.hpp"
#include "renderer/GridRenderer.hpp"
#include "renderer/EditorIconRenderer.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"

namespace Editor {

class ViewportPanel : public Panel {
public:
    ViewportPanel(EditorCamera* camera, Engine::Framebuffer* framebuffer,
                  Engine::DeferredLightingSystem* lightingSystem,
                  Engine::ShadowMapSystem* shadowSystem,
                  Engine::Ref<Engine::Shader> tonemapShader,
                  Engine::Ref<Engine::VertexArray> screenQuadVAO,
                  Engine::DebugRenderer* debugRenderer);

    void OnUpdate(Engine::f32 deltaTime) override;
    void OnImGuiRender() override;
    void OnResize(Engine::u32 width, Engine::u32 height) override;

private:
    void RenderScene();
    void RenderGizmo();
    void RenderViewportToolbar();
    void HandleMousePicking();
    entt::entity PickEntity(const glm::vec2& mousePos);

    EditorCamera* m_Camera;
    Engine::Framebuffer* m_Framebuffer;
    Engine::DeferredLightingSystem* m_LightingSystem;
    Engine::ShadowMapSystem* m_ShadowSystem;
    Engine::Ref<Engine::Shader> m_TonemapShader;
    Engine::Ref<Engine::VertexArray> m_ScreenQuadVAO;
    Engine::DebugRenderer* m_DebugRenderer;
    Engine::Scope<Engine::GridRenderer> m_GridRenderer;
    Engine::Scope<Engine::EditorIconRenderer> m_IconRenderer;

    glm::vec2 m_ViewportSize{0.0f};
    glm::vec2 m_ViewportBounds[2];
    bool m_ViewportSizeChanged = false;
};

} // namespace Editor
