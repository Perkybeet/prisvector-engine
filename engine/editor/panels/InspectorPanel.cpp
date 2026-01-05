#include "InspectorPanel.hpp"
#include "ecs/Core.hpp"
#include "ecs/Components/LightComponents.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Editor {

template<typename T>
bool InspectorPanel::DrawComponentHeader(const char* label, bool canRemove) {
    auto& registry = m_Context->Registry->Raw();
    auto entity = m_Context->SelectedEntity;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                               ImGuiTreeNodeFlags_Framed |
                               ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_AllowOverlap |
                               ImGuiTreeNodeFlags_FramePadding;

    ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    float lineHeight = ImGui::GetFrameHeight();
    ImGui::Separator();

    bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(typeid(T).hash_code()),
                                  flags, "%s", label);
    ImGui::PopStyleVar();

    // Remove button
    if (canRemove) {
        ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
        if (ImGui::Button("X", ImVec2(lineHeight, lineHeight))) {
            registry.remove<T>(entity);
        }
    }

    return open;
}

void InspectorPanel::OnImGuiRender() {
    ImGui::Begin("Inspector");

    if (!m_Context->HasSelection()) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto entity = m_Context->SelectedEntity;

    if (!registry.valid(entity)) {
        m_Context->ClearSelection();
        ImGui::TextDisabled("Invalid entity");
        ImGui::End();
        return;
    }

    // Entity ID
    ImGui::Text("Entity ID: %u", static_cast<Engine::u32>(entity));
    ImGui::Separator();

    // Draw components
    if (registry.all_of<Engine::Transform>(entity)) {
        DrawTransformComponent();
    }

    if (registry.all_of<Engine::MeshComponent>(entity)) {
        DrawMeshComponent();
    }

    if (registry.all_of<Engine::MaterialComponent>(entity)) {
        DrawMaterialComponent();
    }

    if (registry.all_of<Engine::Renderable>(entity)) {
        DrawRenderableComponent();
    }

    if (registry.all_of<Engine::DirectionalLightComponent>(entity)) {
        DrawDirectionalLightComponent();
    }

    if (registry.all_of<Engine::PointLightComponent>(entity)) {
        DrawPointLightComponent();
    }

    if (registry.all_of<Engine::SpotLightComponent>(entity)) {
        DrawSpotLightComponent();
    }

    if (registry.all_of<Engine::AmbientLightComponent>(entity)) {
        DrawAmbientLightComponent();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Add component button
    DrawAddComponentButton();

    ImGui::End();
}

void InspectorPanel::DrawTransformComponent() {
    if (!DrawComponentHeader<Engine::Transform>("Transform", false)) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* transform = registry.try_get<Engine::Transform>(m_Context->SelectedEntity);
    if (!transform) {
        ImGui::TreePop();
        return;
    }

    bool modified = false;

    // Position
    glm::vec3 position = transform->Position;
    if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
        transform->SetPosition(position);
        modified = true;
    }

    // Rotation (as Euler angles in degrees)
    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(transform->Rotation));
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(eulerAngles), 1.0f)) {
        transform->SetRotation(glm::quat(glm::radians(eulerAngles)));
        modified = true;
    }

    // Scale
    glm::vec3 scale = transform->Scale;
    if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f, 0.01f, 100.0f)) {
        transform->SetScale(scale);
        modified = true;
    }

    if (modified) {
        transform->UpdateWorldMatrix();
    }

    ImGui::TreePop();
}

void InspectorPanel::DrawMeshComponent() {
    if (!DrawComponentHeader<Engine::MeshComponent>("Mesh")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* mesh = registry.try_get<Engine::MeshComponent>(m_Context->SelectedEntity);
    if (!mesh) {
        ImGui::TreePop();
        return;
    }

    ImGui::Text("Vertex Count: %u", mesh->VertexCount);
    ImGui::Text("Index Count: %u", mesh->IndexCount);

    if (!mesh->MeshName.empty()) {
        ImGui::Text("Name: %s", mesh->MeshName.c_str());
    }

    ImGui::TreePop();
}

void InspectorPanel::DrawMaterialComponent() {
    if (!DrawComponentHeader<Engine::MaterialComponent>("Material")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* material = registry.try_get<Engine::MaterialComponent>(m_Context->SelectedEntity);
    if (!material) {
        ImGui::TreePop();
        return;
    }

    ImGui::ColorEdit4("Base Color", glm::value_ptr(material->BaseColor));
    ImGui::SliderFloat("Metallic", &material->Metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &material->Roughness, 0.0f, 1.0f);

    ImGui::TreePop();
}

void InspectorPanel::DrawRenderableComponent() {
    if (!DrawComponentHeader<Engine::Renderable>("Renderable")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* renderable = registry.try_get<Engine::Renderable>(m_Context->SelectedEntity);
    if (!renderable) {
        ImGui::TreePop();
        return;
    }

    ImGui::Checkbox("Visible", &renderable->Visible);
    ImGui::Checkbox("Cast Shadows", &renderable->CastShadows);
    ImGui::Checkbox("Receive Shadows", &renderable->ReceiveShadows);

    ImGui::SliderInt("LOD Level", reinterpret_cast<int*>(&renderable->LODLevel), 0, 4);

    ImGui::TreePop();
}

void InspectorPanel::DrawDirectionalLightComponent() {
    if (!DrawComponentHeader<Engine::DirectionalLightComponent>("Directional Light")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* light = registry.try_get<Engine::DirectionalLightComponent>(m_Context->SelectedEntity);
    if (!light) {
        ImGui::TreePop();
        return;
    }

    ImGui::ColorEdit3("Color", glm::value_ptr(light->Color));
    ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("Direction", glm::value_ptr(light->Direction), 0.01f, -1.0f, 1.0f);

    // Normalize direction
    if (glm::length(light->Direction) > 0.001f) {
        light->Direction = glm::normalize(light->Direction);
    }

    ImGui::Checkbox("Cast Shadows", &light->CastShadows);

    if (light->CastShadows) {
        ImGui::DragFloat("Shadow Bias", &light->ShadowBias, 0.0001f, 0.0f, 0.01f, "%.4f");
        ImGui::DragFloat("Normal Bias", &light->NormalBias, 0.001f, 0.0f, 0.1f);
    }

    ImGui::TreePop();
}

void InspectorPanel::DrawPointLightComponent() {
    if (!DrawComponentHeader<Engine::PointLightComponent>("Point Light")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* light = registry.try_get<Engine::PointLightComponent>(m_Context->SelectedEntity);
    if (!light) {
        ImGui::TreePop();
        return;
    }

    ImGui::ColorEdit3("Color", glm::value_ptr(light->Color));
    ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Radius", &light->Radius, 0.1f, 0.1f, 100.0f);

    if (ImGui::CollapsingHeader("Attenuation")) {
        ImGui::DragFloat("Constant", &light->Constant, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Linear", &light->Linear, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Quadratic", &light->Quadratic, 0.0001f, 0.0f, 0.5f, "%.4f");
    }

    ImGui::Checkbox("Enabled", &light->Enabled);

    ImGui::TreePop();
}

void InspectorPanel::DrawSpotLightComponent() {
    if (!DrawComponentHeader<Engine::SpotLightComponent>("Spot Light")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* light = registry.try_get<Engine::SpotLightComponent>(m_Context->SelectedEntity);
    if (!light) {
        ImGui::TreePop();
        return;
    }

    ImGui::ColorEdit3("Color", glm::value_ptr(light->Color));
    ImGui::DragFloat("Intensity", &light->Intensity, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("Direction", glm::value_ptr(light->Direction), 0.01f, -1.0f, 1.0f);

    // Normalize direction
    if (glm::length(light->Direction) > 0.001f) {
        light->Direction = glm::normalize(light->Direction);
    }

    // Convert to degrees for editing
    float innerAngle = glm::degrees(light->InnerCutOff);
    float outerAngle = glm::degrees(light->OuterCutOff);

    if (ImGui::DragFloat("Inner Angle", &innerAngle, 0.5f, 0.0f, outerAngle)) {
        light->InnerCutOff = glm::radians(innerAngle);
    }
    if (ImGui::DragFloat("Outer Angle", &outerAngle, 0.5f, innerAngle, 90.0f)) {
        light->OuterCutOff = glm::radians(outerAngle);
    }

    ImGui::DragFloat("Range", &light->Range, 0.1f, 0.1f, 500.0f);

    ImGui::Checkbox("Cast Shadows", &light->CastShadows);

    ImGui::TreePop();
}

void InspectorPanel::DrawAmbientLightComponent() {
    if (!DrawComponentHeader<Engine::AmbientLightComponent>("Ambient Light")) {
        return;
    }

    auto& registry = m_Context->Registry->Raw();
    auto* light = registry.try_get<Engine::AmbientLightComponent>(m_Context->SelectedEntity);
    if (!light) {
        ImGui::TreePop();
        return;
    }

    ImGui::ColorEdit3("Color", glm::value_ptr(light->Color));
    ImGui::DragFloat("Intensity", &light->Intensity, 0.01f, 0.0f, 10.0f);

    ImGui::TreePop();
}

void InspectorPanel::DrawAddComponentButton() {
    float buttonWidth = ImGui::GetContentRegionAvail().x;

    if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0))) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        auto& registry = m_Context->Registry->Raw();
        auto entity = m_Context->SelectedEntity;

        if (!registry.all_of<Engine::MaterialComponent>(entity)) {
            if (ImGui::MenuItem("Material")) {
                auto& mat = registry.emplace<Engine::MaterialComponent>(entity);
                mat.BaseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
                mat.Metallic = 0.0f;
                mat.Roughness = 0.5f;
            }
        }

        if (!registry.all_of<Engine::Renderable>(entity)) {
            if (ImGui::MenuItem("Renderable")) {
                auto& r = registry.emplace<Engine::Renderable>(entity);
                r.Visible = true;
                r.InFrustum = true;
                r.CastShadows = true;
            }
        }

        ImGui::Separator();

        if (!registry.all_of<Engine::DirectionalLightComponent>(entity)) {
            if (ImGui::MenuItem("Directional Light")) {
                auto& light = registry.emplace<Engine::DirectionalLightComponent>(entity);
                light.Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
                light.Color = glm::vec3(1.0f);
                light.Intensity = 1.0f;
                light.CastShadows = true;
            }
        }

        if (!registry.all_of<Engine::PointLightComponent>(entity)) {
            if (ImGui::MenuItem("Point Light")) {
                auto& light = registry.emplace<Engine::PointLightComponent>(entity);
                light.Color = glm::vec3(1.0f);
                light.Intensity = 1.0f;
                light.Radius = 10.0f;
            }
        }

        if (!registry.all_of<Engine::SpotLightComponent>(entity)) {
            if (ImGui::MenuItem("Spot Light")) {
                auto& light = registry.emplace<Engine::SpotLightComponent>(entity);
                light.Direction = glm::vec3(0.0f, -1.0f, 0.0f);
                light.Color = glm::vec3(1.0f);
                light.Intensity = 1.0f;
                light.InnerCutOff = glm::radians(12.5f);
                light.OuterCutOff = glm::radians(17.5f);
                light.Range = 20.0f;
            }
        }

        if (!registry.all_of<Engine::AmbientLightComponent>(entity)) {
            if (ImGui::MenuItem("Ambient Light")) {
                auto& light = registry.emplace<Engine::AmbientLightComponent>(entity);
                light.Color = glm::vec3(0.1f);
                light.Intensity = 1.0f;
            }
        }

        ImGui::EndPopup();
    }
}

} // namespace Editor
