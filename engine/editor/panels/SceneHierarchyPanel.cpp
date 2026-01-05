#include "SceneHierarchyPanel.hpp"
#include "ecs/Core.hpp"
#include "ecs/Components/LightComponents.hpp"
#include "ecs/Components/NameComponent.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>

namespace Editor {

void SceneHierarchyPanel::OnImGuiRender() {
    ImGui::Begin("Scene Hierarchy");

    auto& registry = m_Context->Registry->Raw();

    // Create entity button
    if (ImGui::Button("+ Create Entity")) {
        ImGui::OpenPopup("CreateEntityPopup");
    }

    if (ImGui::BeginPopup("CreateEntityPopup")) {
        if (ImGui::MenuItem("Empty Entity")) {
            auto entity = registry.create();
            registry.emplace<Engine::Transform>(entity);
            m_Context->Select(entity);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // Count entities for display
    ImGui::TextDisabled("Entities: %zu", m_Context->Registry->EntityCount());

    ImGui::Separator();

    // Draw all entities
    // First, draw root entities (those without Hierarchy or marked as RootEntity)
    auto view = registry.view<Engine::Transform>();
    for (auto entity : view) {
        // Skip if has parent
        auto* hierarchy = registry.try_get<Engine::Hierarchy>(entity);
        if (hierarchy && hierarchy->Parent != entt::null) {
            continue;
        }

        DrawEntityNode(entity);
    }

    // Deselect when clicking empty space
    if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
        m_Context->ClearSelection();
    }

    // Right-click on empty space
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty Entity")) {
            auto entity = registry.create();
            registry.emplace<Engine::Transform>(entity);
            registry.emplace<Engine::NameComponent>(entity).Name = "Empty Entity";
            m_Context->Select(entity);
        }
        ImGui::EndPopup();
    }

    // Drop target for making entities root (unparent)
    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->ContentRegionRect,
                                          ImGui::GetCurrentWindow()->ID)) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
            entt::entity droppedEntity = *static_cast<const entt::entity*>(payload->Data);
            Engine::HierarchyUtils::DetachFromParent(registry, droppedEntity);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(entt::entity entity) {
    auto& registry = m_Context->Registry->Raw();

    // Check if we're renaming this entity
    if (m_RenamingEntity == entity) {
        char buffer[256];
        std::strncpy(buffer, m_RenameBuffer.c_str(), sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        if (ImGui::InputText("##rename", buffer, sizeof(buffer),
                            ImGuiInputTextFlags_EnterReturnsTrue |
                            ImGuiInputTextFlags_AutoSelectAll)) {
            // Apply new name
            auto& name = registry.get_or_emplace<Engine::NameComponent>(entity);
            name.Name = buffer;
            m_RenamingEntity = entt::null;
        }

        // Cancel on escape or click outside
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            (ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered())) {
            m_RenamingEntity = entt::null;
        }

        return;
    }

    Engine::String label = GetEntityDisplayName(entity);

    // Check for children
    bool hasChildren = false;
    auto* hierarchy = registry.try_get<Engine::Hierarchy>(entity);
    if (hierarchy && hierarchy->FirstChild != entt::null) {
        hasChildren = true;
    }

    // Tree node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (m_Context->IsSelected(entity)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<Engine::u64>(entity)),
                                    flags, "%s", label.c_str());

    // Selection on single click
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        m_Context->Select(entity);
    }

    // Start rename on double-click (but not on arrow)
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        m_RenamingEntity = entity;
        m_RenameBuffer = label;
    }

    // Context menu
    DrawEntityContextMenu(entity);

    // Drag and drop source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        ImGui::SetDragDropPayload("ENTITY_DRAG", &entity, sizeof(entt::entity));
        ImGui::Text("%s", label.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag and drop target (for parenting)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
            entt::entity droppedEntity = *static_cast<const entt::entity*>(payload->Data);
            if (droppedEntity != entity) {
                Engine::HierarchyUtils::SetParent(registry, droppedEntity, entity);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Draw children
    if (opened && hasChildren) {
        // Iterate through children
        auto* h = registry.try_get<Engine::Hierarchy>(entity);
        if (h) {
            entt::entity child = h->FirstChild;
            while (child != entt::null) {
                DrawEntityNode(child);
                auto* childHierarchy = registry.try_get<Engine::Hierarchy>(child);
                child = childHierarchy ? childHierarchy->NextSibling : entt::null;
            }
        }
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::DrawEntityContextMenu(entt::entity entity) {
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete Entity")) {
            m_Context->Registry->DestroyEntity(entity);
            if (m_Context->SelectedEntity == entity) {
                m_Context->ClearSelection();
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate")) {
            // TODO: Implement entity duplication
        }

        ImGui::EndPopup();
    }
}

Engine::String SceneHierarchyPanel::GetEntityDisplayName(entt::entity entity) {
    auto& registry = m_Context->Registry->Raw();

    // First check if entity has a custom name
    if (auto* name = registry.try_get<Engine::NameComponent>(entity)) {
        if (!name->Name.empty()) {
            return name->Name;
        }
    }

    // Fallback: check for specific component types
    if (registry.all_of<Engine::DirectionalLightComponent>(entity)) {
        return "Directional Light";
    }
    if (registry.all_of<Engine::PointLightComponent>(entity)) {
        return "Point Light";
    }
    if (registry.all_of<Engine::SpotLightComponent>(entity)) {
        return "Spot Light";
    }
    if (registry.all_of<Engine::AmbientLightComponent>(entity)) {
        return "Ambient Light";
    }
    if (registry.all_of<Engine::MeshComponent>(entity)) {
        auto& mesh = registry.get<Engine::MeshComponent>(entity);
        if (!mesh.MeshName.empty()) {
            return mesh.MeshName;
        }
        return "Mesh Entity";
    }

    // Default name
    return "Entity " + std::to_string(static_cast<Engine::u32>(entity));
}

} // namespace Editor
