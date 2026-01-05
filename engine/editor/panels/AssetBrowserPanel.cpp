#include "AssetBrowserPanel.hpp"
#include "ecs/Core.hpp"
#include <imgui.h>

namespace Editor {

AssetBrowserPanel::AssetBrowserPanel(Engine::Ref<Engine::Mesh> cube,
                                     Engine::Ref<Engine::Mesh> sphere,
                                     Engine::Ref<Engine::Mesh> plane,
                                     Engine::Ref<Engine::Mesh> cylinder)
    : Panel("Asset Browser")
    , m_CubeMesh(cube)
    , m_SphereMesh(sphere)
    , m_PlaneMesh(plane)
    , m_CylinderMesh(cylinder)
{
}

void AssetBrowserPanel::OnImGuiRender() {
    ImGui::Begin("Asset Browser");

    if (ImGui::BeginTabBar("AssetTabs")) {
        if (ImGui::BeginTabItem("Meshes")) {
            DrawMeshesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shaders")) {
            DrawShadersTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Textures")) {
            DrawTexturesTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void AssetBrowserPanel::DrawMeshesTab() {
    ImGui::TextDisabled("Drag to viewport or double-click to create");
    ImGui::Separator();

    // Primitive meshes
    auto DrawMeshItem = [this](const char* name, Engine::Ref<Engine::Mesh> mesh) {
        ImGui::PushID(name);

        bool clicked = ImGui::Selectable(name, false, ImGuiSelectableFlags_AllowDoubleClick);

        // Drag and drop
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("MESH_ASSET", &mesh, sizeof(Engine::Ref<Engine::Mesh>));
            ImGui::Text("Mesh: %s", name);
            ImGui::EndDragDropSource();
        }

        // Double-click to create entity
        if (clicked && ImGui::IsMouseDoubleClicked(0)) {
            auto& registry = m_Context->Registry->Raw();
            auto entity = registry.create();

            auto& t = registry.emplace<Engine::Transform>(entity);
            t.UpdateWorldMatrix();

            auto& mc = registry.emplace<Engine::MeshComponent>(entity);
            mc.VAO = mesh->GetVertexArray();
            mc.IndexCount = mesh->GetIndexCount();
            mc.LocalBounds = mesh->GetBounds();
            mc.MeshName = name;

            auto& mat = registry.emplace<Engine::MaterialComponent>(entity);
            mat.BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
            mat.Metallic = 0.0f;
            mat.Roughness = 0.5f;

            auto& r = registry.emplace<Engine::Renderable>(entity);
            r.Visible = true;
            r.InFrustum = true;
            r.CastShadows = true;

            m_Context->Select(entity);
        }

        ImGui::PopID();
    };

    ImGui::Text("Primitives:");
    ImGui::Indent();
    DrawMeshItem("Cube", m_CubeMesh);
    DrawMeshItem("Sphere", m_SphereMesh);
    DrawMeshItem("Plane", m_PlaneMesh);
    DrawMeshItem("Cylinder", m_CylinderMesh);
    ImGui::Unindent();

    ImGui::Separator();

    // TODO: List loaded model files
    ImGui::TextDisabled("Model files:");
    ImGui::TextDisabled("  (Not implemented yet)");
}

void AssetBrowserPanel::DrawShadersTab() {
    ImGui::TextDisabled("Available shaders:");
    ImGui::Separator();

    // List common shaders
    const char* shaders[] = {
        "basic",
        "instanced",
        "deferred/geometry",
        "deferred/lighting",
        "shadows/depth_csm",
        "shadows/depth_spot",
        "particles/particle_render",
        "postprocess/tonemap"
    };

    for (const auto& shader : shaders) {
        ImGui::BulletText("%s", shader);
    }
}

void AssetBrowserPanel::DrawTexturesTab() {
    ImGui::TextDisabled("Loaded textures:");
    ImGui::Separator();

    // TODO: List loaded textures
    ImGui::TextDisabled("  (Not implemented yet)");
}

} // namespace Editor
