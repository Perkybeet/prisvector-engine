#pragma once

#include "Engine.hpp"
#include "renderer/debug/DebugRenderer.hpp"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Demos {

// Base class for all demos - provides common infrastructure
class DemoBase {
public:
    virtual ~DemoBase() = default;

    // Lifecycle
    virtual void OnInit() = 0;
    virtual void OnShutdown() = 0;
    virtual void OnUpdate(Engine::f32 dt) = 0;
    virtual void OnRender() = 0;
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Engine::Event& e) {}
    virtual void OnResize(Engine::u32 width, Engine::u32 height) {}

    // Info
    virtual const char* GetName() const = 0;
    virtual const char* GetDescription() const = 0;

    // Called by launcher to set window reference
    void SetWindow(Engine::Window* window) { m_Window = window; }
    Engine::Window& GetWindow() { return *m_Window; }

protected:
    // Common resources available to all demos
    entt::registry m_Registry;
    Engine::CameraManager m_CameraManager;
    Engine::Scope<Engine::ShadowMapSystem> m_ShadowSystem;
    Engine::Scope<Engine::DeferredLightingSystem> m_LightingSystem;

    Engine::Ref<Engine::Mesh> m_CubeMesh;
    Engine::Ref<Engine::Mesh> m_SphereMesh;
    Engine::Ref<Engine::Mesh> m_PlaneMesh;
    Engine::Ref<Engine::Mesh> m_CylinderMesh;
    Engine::Ref<Engine::Shader> m_TonemapShader;
    Engine::Ref<Engine::VertexArray> m_ScreenQuadVAO;

    Engine::Window* m_Window = nullptr;

    // Demo state
    float m_Time = 0.0f;
    float m_Exposure = 1.0f;
    bool m_ShowStats = true;
    bool m_ShadowsEnabled = true;
    bool m_ShowHelp = true;

    // Debug renderer for visualizing CSM, GBuffer, etc.
    Engine::Scope<Engine::DebugRenderer> m_DebugRenderer;

    // Helper to initialize common rendering systems
    void InitializeRenderingSystems() {
        auto& resources = Engine::ResourceManager::Instance();

        // Load common meshes
        m_CubeMesh = resources.GetCube();
        m_SphereMesh = resources.GetSphere(48, 24);
        m_PlaneMesh = Engine::MeshLoader::CreatePlane(1.0f, 1.0f, 1);
        m_CylinderMesh = Engine::MeshLoader::CreateCylinder(0.5f, 1.0f, 32);

        // Initialize shadow system
        m_ShadowSystem = Engine::CreateScope<Engine::ShadowMapSystem>();
        m_ShadowSystem->OnCreate(m_Registry);

        // Initialize deferred lighting system
        m_LightingSystem = Engine::CreateScope<Engine::DeferredLightingSystem>();
        m_LightingSystem->OnCreate(m_Registry);
        m_LightingSystem->Resize(m_Window->GetWidth(), m_Window->GetHeight());
        m_LightingSystem->SetShadowSystem(m_ShadowSystem.get());

        // Load tonemap shader
        m_TonemapShader = resources.LoadShader("tonemap", "assets/shaders/postprocess/tonemap.glsl");
        CreateScreenQuad();

        // Initialize debug renderer
        m_DebugRenderer = Engine::CreateScope<Engine::DebugRenderer>();
        m_DebugRenderer->Initialize();
        m_DebugRenderer->SetCSM(m_ShadowSystem->GetCSM());
        m_DebugRenderer->SetGBuffer(&m_LightingSystem->GetGBuffer());
    }

    void ShutdownRenderingSystems() {
        if (m_DebugRenderer) m_DebugRenderer->Shutdown();
        m_ShadowSystem->OnDestroy(m_Registry);
        m_LightingSystem->OnDestroy(m_Registry);
    }

    // Handle debug input (F3 to cycle views, F1 for help)
    void HandleDebugInput() {
        if (Engine::Input::IsKeyJustPressed(Engine::Key::F3)) {
            m_DebugRenderer->CycleView();
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::F1)) {
            m_ShowHelp = !m_ShowHelp;
        }
    }

    // Render debug overlay after main rendering
    void RenderDebugOverlay() {
        if (m_DebugRenderer) {
            m_DebugRenderer->Render(m_Window->GetWidth(), m_Window->GetHeight());
        }
    }

    // Render help panel showing controls
    void RenderHelpPanel() {
        if (!m_ShowHelp) return;

        ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 180),
                                ImGuiCond_FirstUseEver);
        ImGui::Begin("Controls (F1)", &m_ShowHelp,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav);

        ImGui::TextColored(ImVec4(1,1,0,1), "Camera:");
        ImGui::BulletText("WASD - Move");
        ImGui::BulletText("Mouse - Look around");
        ImGui::BulletText("Shift - Sprint");
        ImGui::BulletText("Scroll - Zoom/FOV");

        ImGui::Separator();
        ImGui::TextColored(ImVec4(1,1,0,1), "General:");
        ImGui::BulletText("F1 - Toggle this help");
        ImGui::BulletText("F3 - Cycle debug views");
        ImGui::BulletText("ESC - Exit");
        ImGui::BulletText("+/- - Adjust exposure");

        ImGui::End();
    }

    // Render final image with tonemapping
    void RenderTonemapped() {
        Engine::Framebuffer::BindDefault();
        glViewport(0, 0, m_Window->GetWidth(), m_Window->GetHeight());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        m_TonemapShader->Bind();
        m_TonemapShader->SetInt("u_HDRBuffer", 0);
        m_TonemapShader->SetFloat("u_Exposure", m_Exposure);
        m_TonemapShader->SetFloat("u_Gamma", 2.2f);

        m_LightingSystem->GetLightingBuffer().BindColorTexture(0, 0);

        m_ScreenQuadVAO->Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glEnable(GL_DEPTH_TEST);
    }

    // Render shadows and deferred lighting pass
    void RenderScene() {
        if (auto* cam = m_CameraManager.GetActiveCamera()) {
            m_ShadowSystem->SetCamera(const_cast<Engine::Camera*>(cam));
            m_LightingSystem->SetCamera(const_cast<Engine::Camera*>(cam));
        }

        m_ShadowSystem->OnUpdate(m_Registry, 0.0f);
        m_LightingSystem->OnUpdate(m_Registry, 0.0f);
    }

    // Entity creation helpers
    entt::entity CreateEntity() {
        return m_Registry.create();
    }

    void SetTransform(entt::entity e, const glm::vec3& pos,
                      const glm::vec3& scale = glm::vec3(1.0f),
                      const glm::vec3& rotation = glm::vec3(0.0f)) {
        auto& t = m_Registry.emplace_or_replace<Engine::Transform>(e);
        t.SetPosition(pos);
        t.SetScale(scale);
        if (rotation != glm::vec3(0.0f)) {
            t.SetRotation(glm::quat(rotation));
        }
        t.UpdateWorldMatrix();
    }

    void SetMesh(entt::entity e, Engine::Ref<Engine::Mesh> mesh) {
        auto& mc = m_Registry.emplace_or_replace<Engine::MeshComponent>(e);
        mc.VAO = mesh->GetVertexArray();
        mc.IndexCount = mesh->GetIndexCount();
        mc.LocalBounds = mesh->GetBounds();
    }

    void SetMaterial(entt::entity e, const glm::vec4& color,
                     float metallic = 0.0f, float roughness = 0.5f) {
        auto& mat = m_Registry.emplace_or_replace<Engine::MaterialComponent>(e);
        mat.BaseColor = color;
        mat.Metallic = metallic;
        mat.Roughness = roughness;
    }

    void SetRenderable(entt::entity e, bool castShadows = true) {
        auto& r = m_Registry.emplace_or_replace<Engine::Renderable>(e);
        r.Visible = true;
        r.InFrustum = true;
        r.CastShadows = castShadows;
    }

    // Create a complete renderable entity
    entt::entity CreateObject(Engine::Ref<Engine::Mesh> mesh,
                              const glm::vec3& pos,
                              const glm::vec3& scale,
                              const glm::vec4& color,
                              float metallic = 0.0f,
                              float roughness = 0.5f,
                              bool castShadows = true) {
        auto e = CreateEntity();
        SetTransform(e, pos, scale);
        SetMesh(e, mesh);
        SetMaterial(e, color, metallic, roughness);
        SetRenderable(e, castShadows);
        return e;
    }

    // Light creation helpers
    entt::entity CreateAmbientLight(const glm::vec3& color, float intensity) {
        auto e = CreateEntity();
        auto& light = m_Registry.emplace<Engine::AmbientLightComponent>(e);
        light.Color = color;
        light.Intensity = intensity;
        return e;
    }

    entt::entity CreateDirectionalLight(const glm::vec3& dir, const glm::vec3& color,
                                        float intensity, bool castShadows = true) {
        auto e = CreateEntity();
        auto& light = m_Registry.emplace<Engine::DirectionalLightComponent>(e);
        light.Direction = glm::normalize(dir);
        light.Color = color;
        light.Intensity = intensity;
        light.CastShadows = castShadows;
        return e;
    }

    entt::entity CreatePointLight(const glm::vec3& pos, const glm::vec3& color,
                                  float intensity, float radius) {
        auto e = CreateEntity();

        auto& t = m_Registry.emplace<Engine::Transform>(e);
        t.SetPosition(pos);
        t.UpdateWorldMatrix();

        auto& light = m_Registry.emplace<Engine::PointLightComponent>(e);
        light.Color = color;
        light.Intensity = intensity;
        light.Radius = radius;

        return e;
    }

    entt::entity CreateSpotLight(const glm::vec3& pos, const glm::vec3& dir,
                                 const glm::vec3& color, float intensity,
                                 float innerAngle, float outerAngle, float range,
                                 bool castShadows = true) {
        auto e = CreateEntity();

        auto& t = m_Registry.emplace<Engine::Transform>(e);
        t.SetPosition(pos);
        t.UpdateWorldMatrix();

        auto& light = m_Registry.emplace<Engine::SpotLightComponent>(e);
        light.Direction = glm::normalize(dir);
        light.Color = color;
        light.Intensity = intensity;
        light.InnerCutOff = glm::radians(innerAngle);
        light.OuterCutOff = glm::radians(outerAngle);
        light.Range = range;
        light.CastShadows = castShadows;

        return e;
    }

private:
    void CreateScreenQuad() {
        float vertices[] = {
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f
        };
        Engine::u32 indices[] = {0, 1, 2, 2, 3, 0};

        m_ScreenQuadVAO = Engine::CreateRef<Engine::VertexArray>();
        auto vbo = Engine::CreateRef<Engine::VertexBuffer>(vertices, sizeof(vertices));
        vbo->SetLayout({
            {Engine::ShaderDataType::Float3, "a_Position"},
            {Engine::ShaderDataType::Float2, "a_TexCoords"}
        });
        auto ibo = Engine::CreateRef<Engine::IndexBuffer>(indices, 6);
        m_ScreenQuadVAO->AddVertexBuffer(vbo);
        m_ScreenQuadVAO->SetIndexBuffer(ibo);
    }
};

} // namespace Demos
