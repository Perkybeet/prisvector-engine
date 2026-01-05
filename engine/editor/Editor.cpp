#include "Editor.hpp"
#include "core/Input.hpp"
#include "core/Time.hpp"
#include "core/Logger.hpp"
#include "resources/loaders/MeshLoader.hpp"
#include "ecs/Core.hpp"
#include "ecs/Components/LightComponents.hpp"
#include "ecs/Components/NameComponent.hpp"

// Panels
#include "panels/ViewportPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/InspectorPanel.hpp"
// ToolbarPanel removed - now embedded in ViewportPanel
#include "panels/StatsPanel.hpp"
#include "panels/ConsolePanel.hpp"
#include "panels/RenderSettingsPanel.hpp"
#include "panels/AssetBrowserPanel.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Editor {

EditorApplication::EditorApplication()
    : Engine::Application("Game Engine Editor", 1600, 900)
{
}

void EditorApplication::OnInit() {
    LOG_CORE_INFO("Initializing Editor...");

    // Setup ImGui for docking
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    SetupImGuiStyle();

    // Initialize rendering systems
    InitializeRenderingSystems();

    // Create editor camera
    m_EditorCamera = Engine::CreateScope<EditorCamera>(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    m_EditorCamera->ResetView();

    // Setup editor context
    m_EditorContext.Registry = &m_Registry;
    m_EditorContext.LightingSystem = m_LightingSystem.get();
    m_EditorContext.ShadowSystem = m_ShadowSystem.get();
    m_EditorContext.DebugRenderer = m_DebugRenderer.get();

    // Register entity destruction callback to clear stale selections
    m_Registry.Raw().on_destroy<Engine::Transform>().connect<&EditorApplication::OnEntityDestroyed>(this);

    // Initialize scene snapshot for play mode
    m_SceneSnapshot = Engine::CreateScope<SceneSnapshot>();

    // Create panels
    AddPanel<ViewportPanel>(m_EditorCamera.get(), m_ViewportFramebuffer.get(),
                            m_LightingSystem.get(), m_ShadowSystem.get(),
                            m_TonemapShader, m_ScreenQuadVAO, m_DebugRenderer.get());
    AddPanel<SceneHierarchyPanel>();
    AddPanel<InspectorPanel>();
    AddPanel<StatsPanel>();
    AddPanel<ConsolePanel>();
    AddPanel<RenderSettingsPanel>();
    AddPanel<AssetBrowserPanel>(m_CubeMesh, m_SphereMesh, m_PlaneMesh, m_CylinderMesh);

    // Create default scene
    CreateDefaultScene();

    LOG_CORE_INFO("Editor initialized successfully!");
}

void EditorApplication::OnShutdown() {
    LOG_CORE_INFO("Shutting down Editor...");

    for (auto& panel : m_Panels) {
        panel->OnShutdown();
    }
    m_Panels.clear();

    ShutdownRenderingSystems();
}

void EditorApplication::OnUpdate(Engine::f32 deltaTime) {
    HandleShortcuts();

    // Update panels (camera is updated in OnPostImGuiRender after viewport state is known)
    for (auto& panel : m_Panels) {
        if (panel->IsVisible()) {
            panel->OnUpdate(deltaTime);
        }
    }
}

void EditorApplication::OnRender() {
    // Rendering is handled by ViewportPanel
}

void EditorApplication::OnImGuiRender() {
    SetupDockspace();

    // Render all panels
    for (auto& panel : m_Panels) {
        if (panel->IsVisible()) {
            panel->OnImGuiRender();
        }
    }

    // Debug: Show ImGui demo window
    if (m_ShowDemoWindow) {
        ImGui::ShowDemoWindow(&m_ShowDemoWindow);
    }
}

void EditorApplication::OnPostImGuiRender() {
    // Update camera AFTER ImGui has updated ViewportHovered
    if (m_EditorContext.ViewportHovered && !ImGuizmo::IsUsing()) {
        m_EditorCamera->OnUpdate(Engine::Time::GetDeltaTime());
    }
}

void EditorApplication::OnAppEvent(Engine::Event& e) {
    // Pass events to editor camera
    if (m_EditorContext.ViewportHovered) {
        m_EditorCamera->OnEvent(e);
    }

    // Pass events to panels
    for (auto& panel : m_Panels) {
        panel->OnEvent(e);
    }
}

void EditorApplication::SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Sizes
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ItemInnerSpacing = ImVec2(4, 4);

    // Colors - Dark theme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
}

void EditorApplication::SetupDockspace() {
    static bool dockspaceOpen = true;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
    ImGui::PopStyleVar(3);

    // Dockspace
    ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");

    // Create default layout on first run (when no layout exists)
    static bool firstRun = true;
    if (firstRun && ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        firstRun = false;

        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

        // Create dock layout structure
        ImGuiID dockMain = dockspaceId;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.2f, nullptr, &dockMain);
        ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.25f, nullptr, &dockMain);
        ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.25f, nullptr, &dockMain);
        ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.5f, nullptr, &dockLeft);
        // Split dockRight to put Stats below Inspector/Render Settings
        ImGuiID dockRightBottom = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.3f, nullptr, &dockRight);

        // Assign panels to dock nodes
        ImGui::DockBuilderDockWindow("Viewport", dockMain);
        ImGui::DockBuilderDockWindow("Scene Hierarchy", dockLeft);
        ImGui::DockBuilderDockWindow("Asset Browser", dockLeftBottom);
        ImGui::DockBuilderDockWindow("Inspector", dockRight);
        ImGui::DockBuilderDockWindow("Render Settings", dockRight);
        ImGui::DockBuilderDockWindow("Console", dockBottom);
        ImGui::DockBuilderDockWindow("Stats", dockRightBottom);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Menu bar
    RenderMenuBar();

    ImGui::End();
}

void EditorApplication::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                m_Registry.Clear();
                m_EditorContext.ClearSelection();
                CreateDefaultScene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del", false, m_EditorContext.HasSelection())) {
                if (m_EditorContext.HasSelection()) {
                    m_Registry.DestroyEntity(m_EditorContext.SelectedEntity);
                    m_EditorContext.ClearSelection();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            for (auto& panel : m_Panels) {
                bool visible = panel->IsVisible();
                if (ImGui::MenuItem(panel->GetName().c_str(), nullptr, &visible)) {
                    panel->SetVisible(visible);
                }
            }
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_ShowDemoWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Empty Entity")) {
                auto entity = m_Registry.CreateEntity();
                entity.AddComponent<Engine::Transform>();
                entity.AddComponent<Engine::NameComponent>().Name = "Empty Entity";
                m_EditorContext.Select(entity.GetHandle());
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("3D Object")) {
                if (ImGui::MenuItem("Cube")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Cube";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.UpdateWorldMatrix();
                    auto& mc = entity.AddComponent<Engine::MeshComponent>();
                    mc.VAO = m_CubeMesh->GetVertexArray();
                    mc.IndexCount = m_CubeMesh->GetIndexCount();
                    mc.LocalBounds = m_CubeMesh->GetBounds();
                    auto& mat = entity.AddComponent<Engine::MaterialComponent>();
                    mat.BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                    mat.Metallic = 0.0f;
                    mat.Roughness = 0.5f;
                    auto& r = entity.AddComponent<Engine::Renderable>();
                    r.Visible = true;
                    r.InFrustum = true;
                    r.CastShadows = true;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Sphere")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Sphere";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.UpdateWorldMatrix();
                    auto& mc = entity.AddComponent<Engine::MeshComponent>();
                    mc.VAO = m_SphereMesh->GetVertexArray();
                    mc.IndexCount = m_SphereMesh->GetIndexCount();
                    mc.LocalBounds = m_SphereMesh->GetBounds();
                    auto& mat = entity.AddComponent<Engine::MaterialComponent>();
                    mat.BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                    mat.Metallic = 0.0f;
                    mat.Roughness = 0.5f;
                    auto& r = entity.AddComponent<Engine::Renderable>();
                    r.Visible = true;
                    r.InFrustum = true;
                    r.CastShadows = true;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Plane")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Plane";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.SetScale(glm::vec3(10.0f, 1.0f, 10.0f));
                    t.UpdateWorldMatrix();
                    auto& mc = entity.AddComponent<Engine::MeshComponent>();
                    mc.VAO = m_PlaneMesh->GetVertexArray();
                    mc.IndexCount = m_PlaneMesh->GetIndexCount();
                    mc.LocalBounds = m_PlaneMesh->GetBounds();
                    auto& mat = entity.AddComponent<Engine::MaterialComponent>();
                    mat.BaseColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                    mat.Metallic = 0.0f;
                    mat.Roughness = 0.8f;
                    auto& r = entity.AddComponent<Engine::Renderable>();
                    r.Visible = true;
                    r.InFrustum = true;
                    r.CastShadows = false;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Cylinder")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Cylinder";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.UpdateWorldMatrix();
                    auto& mc = entity.AddComponent<Engine::MeshComponent>();
                    mc.VAO = m_CylinderMesh->GetVertexArray();
                    mc.IndexCount = m_CylinderMesh->GetIndexCount();
                    mc.LocalBounds = m_CylinderMesh->GetBounds();
                    auto& mat = entity.AddComponent<Engine::MaterialComponent>();
                    mat.BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                    mat.Metallic = 0.0f;
                    mat.Roughness = 0.5f;
                    auto& r = entity.AddComponent<Engine::Renderable>();
                    r.Visible = true;
                    r.InFrustum = true;
                    r.CastShadows = true;
                    m_EditorContext.Select(entity.GetHandle());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Directional Light";
                    entity.AddComponent<Engine::Transform>();
                    auto& light = entity.AddComponent<Engine::DirectionalLightComponent>();
                    light.Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
                    light.Color = glm::vec3(1.0f);
                    light.Intensity = 1.0f;
                    light.CastShadows = true;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Point Light")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Point Light";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
                    t.UpdateWorldMatrix();
                    auto& light = entity.AddComponent<Engine::PointLightComponent>();
                    light.Color = glm::vec3(1.0f);
                    light.Intensity = 1.0f;
                    light.Radius = 10.0f;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Spot Light")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Spot Light";
                    auto& t = entity.AddComponent<Engine::Transform>();
                    t.SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
                    t.UpdateWorldMatrix();
                    auto& light = entity.AddComponent<Engine::SpotLightComponent>();
                    light.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
                    light.Color = glm::vec3(1.0f);
                    light.Intensity = 1.0f;
                    light.InnerCutOff = glm::radians(12.5f);
                    light.OuterCutOff = glm::radians(17.5f);
                    light.Range = 20.0f;
                    light.CastShadows = true;
                    m_EditorContext.Select(entity.GetHandle());
                }
                if (ImGui::MenuItem("Ambient Light")) {
                    auto entity = m_Registry.CreateEntity();
                    entity.AddComponent<Engine::NameComponent>().Name = "Ambient Light";
                    entity.AddComponent<Engine::Transform>();
                    auto& light = entity.AddComponent<Engine::AmbientLightComponent>();
                    light.Color = glm::vec3(0.1f);
                    light.Intensity = 1.0f;
                    m_EditorContext.Select(entity.GetHandle());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // FPS display on the right
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);
        ImGui::Text("FPS: %.1f", Engine::Time::GetFPS());

        ImGui::EndMenuBar();
    }
}

void EditorApplication::HandleShortcuts() {
    // Gizmo mode shortcuts
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        if (Engine::Input::IsKeyJustPressed(Engine::Key::W)) {
            m_EditorContext.CurrentGizmoMode = GizmoMode::Translate;
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::E)) {
            m_EditorContext.CurrentGizmoMode = GizmoMode::Rotate;
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::R)) {
            m_EditorContext.CurrentGizmoMode = GizmoMode::Scale;
        }

        // Delete selected entity
        if (Engine::Input::IsKeyJustPressed(Engine::Key::Delete)) {
            if (m_EditorContext.HasSelection()) {
                m_Registry.DestroyEntity(m_EditorContext.SelectedEntity);
                m_EditorContext.ClearSelection();
            }
        }

        // Focus on selected (F key)
        if (Engine::Input::IsKeyJustPressed(Engine::Key::F)) {
            if (m_EditorContext.HasSelection()) {
                auto* transform = m_Registry.Raw().try_get<Engine::Transform>(m_EditorContext.SelectedEntity);
                if (transform) {
                    m_EditorCamera->FocusOn(transform->Position, 5.0f);
                }
            }
        }

        // Debug view cycling
        if (Engine::Input::IsKeyJustPressed(Engine::Key::F3)) {
            m_DebugRenderer->CycleView();
            m_EditorContext.CurrentDebugView = m_DebugRenderer->GetActiveView();
        }
    }
}

void EditorApplication::InitializeRenderingSystems() {
    auto& resources = Engine::ResourceManager::Instance();

    // Load common meshes
    m_CubeMesh = resources.GetCube();
    m_SphereMesh = resources.GetSphere(32, 16);
    m_PlaneMesh = Engine::MeshLoader::CreatePlane(1.0f, 1.0f, 1);
    m_CylinderMesh = Engine::MeshLoader::CreateCylinder(0.5f, 1.0f, 32);

    // Initialize shadow system
    m_ShadowSystem = Engine::CreateScope<Engine::ShadowMapSystem>();
    m_ShadowSystem->OnCreate(m_Registry.Raw());

    // Initialize deferred lighting system
    m_LightingSystem = Engine::CreateScope<Engine::DeferredLightingSystem>();
    m_LightingSystem->OnCreate(m_Registry.Raw());
    m_LightingSystem->Resize(GetWindow().GetWidth(), GetWindow().GetHeight());
    m_LightingSystem->SetShadowSystem(m_ShadowSystem.get());

    // Load tonemap shader
    m_TonemapShader = resources.LoadShader("tonemap", "assets/shaders/postprocess/tonemap.glsl");

    // Create screen quad
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

    // Create viewport framebuffer
    Engine::FramebufferSpecification fbSpec;
    fbSpec.Width = GetWindow().GetWidth();
    fbSpec.Height = GetWindow().GetHeight();
    fbSpec.Attachments = {Engine::FramebufferTextureFormat::RGBA16F,
                          Engine::FramebufferTextureFormat::Depth24Stencil8};
    m_ViewportFramebuffer = Engine::CreateScope<Engine::Framebuffer>(fbSpec);

    // Initialize debug renderer
    m_DebugRenderer = Engine::CreateScope<Engine::DebugRenderer>();
    m_DebugRenderer->Initialize();
    m_DebugRenderer->SetCSM(m_ShadowSystem->GetCSM());
    m_DebugRenderer->SetGBuffer(&m_LightingSystem->GetGBuffer());
}

void EditorApplication::ShutdownRenderingSystems() {
    if (m_DebugRenderer) m_DebugRenderer->Shutdown();
    if (m_ShadowSystem) m_ShadowSystem->OnDestroy(m_Registry.Raw());
    if (m_LightingSystem) m_LightingSystem->OnDestroy(m_Registry.Raw());
}

void EditorApplication::CreateDefaultScene() {
    // Create ambient light
    {
        auto entity = m_Registry.CreateEntity();
        entity.AddComponent<Engine::Transform>();
        auto& light = entity.AddComponent<Engine::AmbientLightComponent>();
        light.Color = glm::vec3(0.1f, 0.1f, 0.12f);
        light.Intensity = 1.0f;
    }

    // Create directional light (sun)
    {
        auto entity = m_Registry.CreateEntity();
        entity.AddComponent<Engine::Transform>();
        auto& light = entity.AddComponent<Engine::DirectionalLightComponent>();
        light.Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
        light.Color = glm::vec3(1.0f, 0.95f, 0.9f);
        light.Intensity = 1.5f;
        light.CastShadows = true;
    }

    // Create ground plane
    {
        auto entity = m_Registry.CreateEntity();
        auto& t = entity.AddComponent<Engine::Transform>();
        t.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        t.SetScale(glm::vec3(20.0f, 1.0f, 20.0f));
        t.UpdateWorldMatrix();

        auto& mc = entity.AddComponent<Engine::MeshComponent>();
        mc.VAO = m_PlaneMesh->GetVertexArray();
        mc.IndexCount = m_PlaneMesh->GetIndexCount();
        mc.LocalBounds = m_PlaneMesh->GetBounds();

        auto& mat = entity.AddComponent<Engine::MaterialComponent>();
        mat.BaseColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        mat.Metallic = 0.0f;
        mat.Roughness = 0.9f;

        auto& r = entity.AddComponent<Engine::Renderable>();
        r.Visible = true;
        r.InFrustum = true;
        r.CastShadows = false;
    }

    // Create a few sample cubes
    for (int i = 0; i < 3; ++i) {
        auto entity = m_Registry.CreateEntity();
        auto& t = entity.AddComponent<Engine::Transform>();
        t.SetPosition(glm::vec3(-3.0f + i * 3.0f, 0.5f, 0.0f));
        t.UpdateWorldMatrix();

        auto& mc = entity.AddComponent<Engine::MeshComponent>();
        mc.VAO = m_CubeMesh->GetVertexArray();
        mc.IndexCount = m_CubeMesh->GetIndexCount();
        mc.LocalBounds = m_CubeMesh->GetBounds();

        auto& mat = entity.AddComponent<Engine::MaterialComponent>();
        mat.BaseColor = glm::vec4(0.2f + i * 0.3f, 0.3f, 0.8f - i * 0.2f, 1.0f);
        mat.Metallic = i * 0.3f;
        mat.Roughness = 0.3f + i * 0.2f;

        auto& r = entity.AddComponent<Engine::Renderable>();
        r.Visible = true;
        r.InFrustum = true;
        r.CastShadows = true;
    }

    // Create a sphere
    {
        auto entity = m_Registry.CreateEntity();
        auto& t = entity.AddComponent<Engine::Transform>();
        t.SetPosition(glm::vec3(0.0f, 1.5f, 3.0f));
        t.UpdateWorldMatrix();

        auto& mc = entity.AddComponent<Engine::MeshComponent>();
        mc.VAO = m_SphereMesh->GetVertexArray();
        mc.IndexCount = m_SphereMesh->GetIndexCount();
        mc.LocalBounds = m_SphereMesh->GetBounds();

        auto& mat = entity.AddComponent<Engine::MaterialComponent>();
        mat.BaseColor = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
        mat.Metallic = 0.9f;
        mat.Roughness = 0.1f;

        auto& r = entity.AddComponent<Engine::Renderable>();
        r.Visible = true;
        r.InFrustum = true;
        r.CastShadows = true;
    }

    LOG_CORE_INFO("Default scene created with {} entities", m_Registry.EntityCount());
}

void EditorApplication::OnPlayButtonPressed() {
    if (m_EditorContext.State == PlayState::Edit) {
        // Starting play mode - save scene state
        m_SceneSnapshot->Capture(m_Registry);
        m_EditorContext.State = PlayState::Play;
        Engine::Time::SetTimeScale(1.0f);
        LOG_CORE_INFO("Entered Play mode");
    } else if (m_EditorContext.State == PlayState::Pause) {
        // Resume from pause
        m_EditorContext.State = PlayState::Play;
        Engine::Time::SetTimeScale(1.0f);
        LOG_CORE_INFO("Resumed Play mode");
    }
}

void EditorApplication::OnPauseButtonPressed() {
    if (m_EditorContext.State == PlayState::Play) {
        m_EditorContext.State = PlayState::Pause;
        Engine::Time::SetTimeScale(0.0f);
        LOG_CORE_INFO("Paused");
    } else if (m_EditorContext.State == PlayState::Pause) {
        m_EditorContext.State = PlayState::Play;
        Engine::Time::SetTimeScale(1.0f);
        LOG_CORE_INFO("Resumed from pause");
    }
}

void EditorApplication::OnStopButtonPressed() {
    if (m_EditorContext.State != PlayState::Edit) {
        // Restore scene state
        if (m_SceneSnapshot->HasSnapshot()) {
            m_SceneSnapshot->Restore(m_Registry);
            m_EditorContext.ClearSelection();
            LOG_CORE_INFO("Scene restored to pre-play state");
        }
        m_EditorContext.State = PlayState::Edit;
        Engine::Time::SetTimeScale(1.0f);
        LOG_CORE_INFO("Exited Play mode");
    }
}

void EditorApplication::OnEntityDestroyed(entt::registry& registry, entt::entity entity) {
    (void)registry;

    // Clear primary selection if it matches destroyed entity
    if (m_EditorContext.SelectedEntity == entity) {
        m_EditorContext.SelectedEntity = entt::null;
    }

    // Remove from multi-selection
    auto& multi = m_EditorContext.MultiSelection;
    multi.erase(std::remove(multi.begin(), multi.end(), entity), multi.end());

    // Update primary selection to first in multi-selection if available
    if (m_EditorContext.SelectedEntity == entt::null && !multi.empty()) {
        m_EditorContext.SelectedEntity = multi.front();
    }
}

} // namespace Editor
