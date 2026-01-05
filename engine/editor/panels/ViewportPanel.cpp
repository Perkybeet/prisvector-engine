#include "ViewportPanel.hpp"
#include "ecs/Core.hpp"
#include "core/Input.hpp"
#include "math/Ray.hpp"
#include <imgui.h>
#include <ImGuizmo.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <limits>

namespace Editor {

ViewportPanel::ViewportPanel(EditorCamera* camera, Engine::Framebuffer* framebuffer,
                             Engine::DeferredLightingSystem* lightingSystem,
                             Engine::ShadowMapSystem* shadowSystem,
                             Engine::Ref<Engine::Shader> tonemapShader,
                             Engine::Ref<Engine::VertexArray> screenQuadVAO,
                             Engine::DebugRenderer* debugRenderer)
    : Panel("Viewport")
    , m_Camera(camera)
    , m_Framebuffer(framebuffer)
    , m_LightingSystem(lightingSystem)
    , m_ShadowSystem(shadowSystem)
    , m_TonemapShader(tonemapShader)
    , m_ScreenQuadVAO(screenQuadVAO)
    , m_DebugRenderer(debugRenderer)
{
    m_GridRenderer = Engine::CreateScope<Engine::GridRenderer>();
}

void ViewportPanel::OnUpdate(Engine::f32 deltaTime) {
    (void)deltaTime;

    if (m_ViewportSizeChanged && m_ViewportSize.x > 0 && m_ViewportSize.y > 0) {
        m_Framebuffer->Resize(static_cast<Engine::u32>(m_ViewportSize.x),
                              static_cast<Engine::u32>(m_ViewportSize.y));
        m_LightingSystem->Resize(static_cast<Engine::u32>(m_ViewportSize.x),
                                  static_cast<Engine::u32>(m_ViewportSize.y));
        m_Camera->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
        m_ViewportSizeChanged = false;
    }
}

void ViewportPanel::RenderScene() {
    auto& registry = m_Context->Registry->Raw();

    // Set camera for systems
    m_ShadowSystem->SetCamera(const_cast<Engine::Camera*>(&m_Camera->GetCamera()));
    m_LightingSystem->SetCamera(const_cast<Engine::Camera*>(&m_Camera->GetCamera()));

    // Shadow pass
    if (m_Context->ShadowsEnabled) {
        m_ShadowSystem->OnUpdate(registry, 0.0f);
    }

    // Deferred lighting pass
    m_LightingSystem->OnUpdate(registry, 0.0f);

    // Tonemap to viewport framebuffer
    m_Framebuffer->Bind();
    glViewport(0, 0, static_cast<GLsizei>(m_ViewportSize.x), static_cast<GLsizei>(m_ViewportSize.y));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_TonemapShader->Bind();
    m_TonemapShader->SetInt("u_HDRBuffer", 0);
    m_TonemapShader->SetFloat("u_Exposure", m_Context->Exposure);
    m_TonemapShader->SetFloat("u_Gamma", m_Context->Gamma);

    m_LightingSystem->GetLightingBuffer().BindColorTexture(0, 0);

    m_ScreenQuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glEnable(GL_DEPTH_TEST);

    // Render grid
    if (m_GridRenderer && m_GridRenderer->IsVisible()) {
        glm::mat4 vp = m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix();
        m_GridRenderer->Render(vp, m_Camera->GetPosition());
    }

    // Debug overlay
    if (m_Context->CurrentDebugView != Engine::DebugView::None) {
        m_DebugRenderer->Render(static_cast<Engine::u32>(m_ViewportSize.x),
                                static_cast<Engine::u32>(m_ViewportSize.y));
    }

    m_Framebuffer->Unbind();
}

void ViewportPanel::RenderGizmo() {
    if (!m_Context->HasSelection()) return;

    auto& registry = m_Context->Registry->Raw();
    auto* transform = registry.try_get<Engine::Transform>(m_Context->SelectedEntity);
    if (!transform) return;

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    // Use viewport bounds (content region) instead of full window
    // This accounts for the toolbar at the top
    ImGuizmo::SetRect(m_Context->ViewportBounds[0].x, m_Context->ViewportBounds[0].y,
                      m_ViewportSize.x, m_ViewportSize.y);

    // Get camera matrices
    const glm::mat4& view = m_Camera->GetViewMatrix();
    const glm::mat4& projection = m_Camera->GetProjectionMatrix();

    // Get transform matrix
    glm::mat4 transformMatrix = transform->WorldMatrix;

    // Determine gizmo operation
    ImGuizmo::OPERATION operation;
    switch (m_Context->CurrentGizmoMode) {
        case GizmoMode::Translate: operation = ImGuizmo::TRANSLATE; break;
        case GizmoMode::Rotate:    operation = ImGuizmo::ROTATE; break;
        case GizmoMode::Scale:     operation = ImGuizmo::SCALE; break;
        default: operation = ImGuizmo::TRANSLATE; break;
    }

    // Determine gizmo mode (local/world)
    ImGuizmo::MODE mode = (m_Context->CurrentGizmoSpace == GizmoSpace::Local)
        ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    // Snapping
    float* snapValues = nullptr;
    float snapTranslate[3] = {m_Context->TranslateSnapValue.x,
                              m_Context->TranslateSnapValue.y,
                              m_Context->TranslateSnapValue.z};
    float snapRotate[3] = {m_Context->RotateSnapValue,
                           m_Context->RotateSnapValue,
                           m_Context->RotateSnapValue};
    float snapScale[3] = {m_Context->ScaleSnapValue,
                          m_Context->ScaleSnapValue,
                          m_Context->ScaleSnapValue};

    if (m_Context->GizmoSnapping) {
        switch (m_Context->CurrentGizmoMode) {
            case GizmoMode::Translate: snapValues = snapTranslate; break;
            case GizmoMode::Rotate:    snapValues = snapRotate; break;
            case GizmoMode::Scale:     snapValues = snapScale; break;
        }
    }

    // Manipulate
    if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                             operation, mode, glm::value_ptr(transformMatrix),
                             nullptr, snapValues)) {
        // Decompose and apply
        glm::vec3 translation, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        glm::decompose(transformMatrix, scale, rotation, translation, skew, perspective);

        transform->SetPosition(translation);
        transform->SetRotation(rotation);
        transform->SetScale(scale);
        transform->UpdateWorldMatrix();
    }
}

void ViewportPanel::RenderViewportToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    float buttonSize = 24.0f;

    // Horizontal container for toolbar
    ImGui::BeginChild("ViewportToolbar", ImVec2(0, buttonSize + 8), false,
        ImGuiWindowFlags_NoScrollbar);

    // Gizmo mode buttons (W/E/R = Translate/Rotate/Scale)
    auto DrawModeButton = [&](const char* label, GizmoMode mode, const char* tooltip) {
        bool active = m_Context->CurrentGizmoMode == mode;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button(label, ImVec2(buttonSize, buttonSize))) {
            m_Context->CurrentGizmoMode = mode;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        if (active) ImGui::PopStyleColor();
    };

    DrawModeButton("W", GizmoMode::Translate, "Move (W)");
    ImGui::SameLine();
    DrawModeButton("E", GizmoMode::Rotate, "Rotate (E)");
    ImGui::SameLine();
    DrawModeButton("R", GizmoMode::Scale, "Scale (R)");

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // World/Local toggle
    const char* spaceLabel = (m_Context->CurrentGizmoSpace == GizmoSpace::World) ? "World" : "Local";
    if (ImGui::Button(spaceLabel, ImVec2(45, buttonSize))) {
        m_Context->CurrentGizmoSpace = (m_Context->CurrentGizmoSpace == GizmoSpace::World)
            ? GizmoSpace::Local : GizmoSpace::World;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle World/Local space");

    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();

    // Snap toggle
    ImGui::Checkbox("Snap", &m_Context->GizmoSnapping);

    if (m_Context->GizmoSnapping) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        switch (m_Context->CurrentGizmoMode) {
            case GizmoMode::Translate:
                ImGui::DragFloat("##snapT", &m_Context->TranslateSnapValue.x, 0.1f, 0.1f, 10.0f);
                m_Context->TranslateSnapValue.y = m_Context->TranslateSnapValue.z = m_Context->TranslateSnapValue.x;
                break;
            case GizmoMode::Rotate:
                ImGui::DragFloat("##snapR", &m_Context->RotateSnapValue, 1.0f, 1.0f, 90.0f);
                break;
            case GizmoMode::Scale:
                ImGui::DragFloat("##snapS", &m_Context->ScaleSnapValue, 0.1f, 0.1f, 2.0f);
                break;
        }
    }

    // Right section - Play/Pause/Stop
    float rightSectionWidth = 60.0f;
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - rightSectionWidth + 8);

    // Play/Pause button
    bool isPlaying = m_Context->State == PlayState::Play;
    if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button(isPlaying ? "||" : ">", ImVec2(buttonSize, buttonSize))) {
        if (m_Context->State == PlayState::Edit) {
            m_Context->State = PlayState::Play;
        } else if (m_Context->State == PlayState::Play) {
            m_Context->State = PlayState::Pause;
        } else {
            m_Context->State = PlayState::Play;
        }
    }
    if (isPlaying) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play/Pause");

    ImGui::SameLine();

    // Stop button
    bool canStop = m_Context->State != PlayState::Edit;
    if (!canStop) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    if (ImGui::Button("[]", ImVec2(buttonSize, buttonSize)) && canStop) {
        m_Context->State = PlayState::Edit;
    }
    if (!canStop) ImGui::PopStyleVar();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Stop");

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

void ViewportPanel::OnImGuiRender() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    // Render toolbar at top
    RenderViewportToolbar();

    // Update focus/hover state
    m_Context->ViewportFocused = ImGui::IsWindowFocused();
    m_Context->ViewportHovered = ImGui::IsWindowHovered();

    // Get viewport size
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (viewportPanelSize.x != m_ViewportSize.x || viewportPanelSize.y != m_ViewportSize.y) {
        m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
        m_Context->ViewportSize = m_ViewportSize;
        m_ViewportSizeChanged = true;
    }

    // Render scene to framebuffer
    if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0) {
        RenderScene();

        // Display framebuffer texture
        Engine::u32 textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(textureID)),
                     ImVec2(m_ViewportSize.x, m_ViewportSize.y),
                     ImVec2(0, 1), ImVec2(1, 0));

        // Get the exact rect of the viewport image (for gizmo and picking)
        ImVec2 imageMin = ImGui::GetItemRectMin();
        ImVec2 imageMax = ImGui::GetItemRectMax();
        m_Context->ViewportBounds[0] = {imageMin.x, imageMin.y};
        m_Context->ViewportBounds[1] = {imageMax.x, imageMax.y};

        // Render gizmo over the image
        RenderGizmo();
    }

    // Handle mouse picking
    HandleMousePicking();

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::OnResize(Engine::u32 width, Engine::u32 height) {
    (void)width;
    (void)height;
}

entt::entity ViewportPanel::PickEntity(const glm::vec2& mousePos) {
    auto& registry = m_Context->Registry->Raw();

    // Create ray from camera through mouse position
    Engine::Ray ray = Engine::Ray::FromScreenPoint(
        mousePos,
        m_Context->ViewportBounds[0],
        m_ViewportSize,
        m_Camera->GetViewMatrix(),
        m_Camera->GetProjectionMatrix()
    );

    // Find closest intersected entity
    entt::entity closestEntity = entt::null;
    float closestDistance = std::numeric_limits<float>::max();

    // Iterate all renderable entities with mesh bounds
    auto view = registry.view<Engine::Transform, Engine::Renderable, Engine::MeshComponent>();
    for (auto entity : view) {
        auto& renderable = view.get<Engine::Renderable>(entity);
        if (!renderable.Visible) continue;

        auto& mesh = view.get<Engine::MeshComponent>(entity);
        auto& transform = view.get<Engine::Transform>(entity);

        // Transform local bounds to world space
        Engine::AABB worldBounds = mesh.LocalBounds.Transform(transform.WorldMatrix);

        float hitDistance;
        if (ray.IntersectsAABB(worldBounds, hitDistance)) {
            if (hitDistance < closestDistance) {
                closestDistance = hitDistance;
                closestEntity = entity;
            }
        }
    }

    return closestEntity;
}

void ViewportPanel::HandleMousePicking() {
    // Only pick when hovering viewport and not using gizmo
    if (!m_Context->ViewportHovered) return;
    if (ImGuizmo::IsOver() || ImGuizmo::IsUsing()) return;

    // Don't pick when Alt is pressed (Alt+click is for camera)
    bool altPressed = Engine::Input::IsKeyPressed(Engine::Key::LeftAlt) ||
                      Engine::Input::IsKeyPressed(Engine::Key::RightAlt);
    if (altPressed) return;

    // Left click to select
    if (Engine::Input::IsMouseButtonJustPressed(Engine::Mouse::Left)) {
        glm::vec2 mousePos = Engine::Input::GetMousePosition();

        // Check if mouse is within viewport bounds
        if (mousePos.x >= m_Context->ViewportBounds[0].x &&
            mousePos.x <= m_Context->ViewportBounds[1].x &&
            mousePos.y >= m_Context->ViewportBounds[0].y &&
            mousePos.y <= m_Context->ViewportBounds[1].y) {

            entt::entity picked = PickEntity(mousePos);

            if (picked != entt::null) {
                // Check for multi-select (Ctrl held)
                if (Engine::Input::IsKeyPressed(Engine::Key::LeftControl)) {
                    if (m_Context->IsSelected(picked)) {
                        // Deselect if already selected
                        auto& multi = m_Context->MultiSelection;
                        multi.erase(std::remove(multi.begin(), multi.end(), picked), multi.end());
                        if (m_Context->SelectedEntity == picked) {
                            m_Context->SelectedEntity = multi.empty() ? entt::null : multi.front();
                        }
                    } else {
                        m_Context->AddToSelection(picked);
                    }
                } else {
                    m_Context->Select(picked);
                }
            } else {
                // Clicked on nothing - clear selection (unless Ctrl is held)
                if (!Engine::Input::IsKeyPressed(Engine::Key::LeftControl)) {
                    m_Context->ClearSelection();
                }
            }
        }
    }
}

} // namespace Editor
