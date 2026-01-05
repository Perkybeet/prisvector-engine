#pragma once

#include "core/Application.hpp"
#include "EditorContext.hpp"
#include "EditorCamera.hpp"
#include "SceneSnapshot.hpp"
#include "panels/Panel.hpp"
#include "renderer/pipeline/Framebuffer.hpp"
#include "renderer/lighting/DeferredLightingSystem.hpp"
#include "renderer/shadows/ShadowMapSystem.hpp"
#include "renderer/debug/DebugRenderer.hpp"
#include "resources/ResourceManager.hpp"

namespace Editor {

class EditorApplication : public Engine::Application {
public:
    EditorApplication();
    virtual ~EditorApplication() = default;

protected:
    void OnInit() override;
    void OnShutdown() override;
    void OnUpdate(Engine::f32 deltaTime) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnPostImGuiRender() override;
    void OnAppEvent(Engine::Event& e) override;

private:
    void SetupImGuiStyle();
    void SetupDockspace();
    void RenderMenuBar();
    void HandleShortcuts();
    void CreateDefaultScene();
    void InitializeRenderingSystems();
    void ShutdownRenderingSystems();

    template<typename T, typename... Args>
    T* AddPanel(Args&&... args) {
        auto panel = Engine::CreateScope<T>(std::forward<Args>(args)...);
        T* ptr = panel.get();
        panel->OnInit(m_EditorContext);
        m_Panels.push_back(std::move(panel));
        return ptr;
    }

    // Entity destruction callback to clear stale selections
    void OnEntityDestroyed(entt::registry& registry, entt::entity entity);

    // Play mode controls
    void OnPlayButtonPressed();
    void OnPauseButtonPressed();
    void OnStopButtonPressed();

private:
    EditorContext m_EditorContext;
    Engine::Vector<Engine::Scope<Panel>> m_Panels;

    // Editor camera
    Engine::Scope<EditorCamera> m_EditorCamera;

    // Rendering systems
    Engine::Scope<Engine::DeferredLightingSystem> m_LightingSystem;
    Engine::Scope<Engine::ShadowMapSystem> m_ShadowSystem;
    Engine::Scope<Engine::DebugRenderer> m_DebugRenderer;

    // Resources
    Engine::Ref<Engine::Mesh> m_CubeMesh;
    Engine::Ref<Engine::Mesh> m_SphereMesh;
    Engine::Ref<Engine::Mesh> m_PlaneMesh;
    Engine::Ref<Engine::Mesh> m_CylinderMesh;
    Engine::Ref<Engine::Shader> m_TonemapShader;
    Engine::Ref<Engine::VertexArray> m_ScreenQuadVAO;

    // Viewport framebuffer
    Engine::Scope<Engine::Framebuffer> m_ViewportFramebuffer;

    // Scene snapshot for play mode
    Engine::Scope<SceneSnapshot> m_SceneSnapshot;

    bool m_ShowDemoWindow = false;
};

} // namespace Editor
