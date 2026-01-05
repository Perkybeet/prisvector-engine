#include "StatsPanel.hpp"
#include "core/Time.hpp"
#include "ecs/Components/Transform.hpp"
#include "ecs/Components/Renderable.hpp"
#include "ecs/Components/LightComponents.hpp"
#include <imgui.h>

namespace Editor {

void StatsPanel::OnImGuiRender() {
    ImGui::Begin("Statistics");

    // Update FPS smoothing
    Engine::f32 deltaTime = Engine::Time::GetDeltaTime();
    m_FrameTimes.push_back(deltaTime);
    if (m_FrameTimes.size() > SampleCount) {
        m_FrameTimes.erase(m_FrameTimes.begin());
    }

    Engine::f32 currentTime = Engine::Time::GetTime();
    if (currentTime - m_LastFPSUpdate >= UpdateInterval) {
        if (!m_FrameTimes.empty()) {
            Engine::f32 sum = 0.0f;
            for (Engine::f32 dt : m_FrameTimes) {
                sum += dt;
            }
            m_SmoothedFPS = static_cast<Engine::f32>(m_FrameTimes.size()) / sum;
            m_SmoothedFrameTime = (sum / static_cast<Engine::f32>(m_FrameTimes.size())) * 1000.0f;
        }
        m_LastFPSUpdate = currentTime;
    }

    // Frame stats
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", m_SmoothedFPS);
        ImGui::Text("Frame Time: %.2f ms", m_SmoothedFrameTime);
        ImGui::Text("Total Time: %.1f s", Engine::Time::GetTime());
    }

    // Entity stats
    if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& registry = m_Context->Registry->Raw();

        ImGui::Text("Total Entities: %zu", m_Context->Registry->EntityCount());

        // Component counts
        Engine::usize transformCount = 0;
        Engine::usize renderableCount = 0;

        auto transformView = registry.view<Engine::Transform>();
        transformCount = transformView.size();

        auto renderableView = registry.view<Engine::Renderable>();
        renderableCount = renderableView.size();

        ImGui::Text("  Transforms: %zu", transformCount);
        ImGui::Text("  Renderables: %zu", renderableCount);
    }

    // Light stats
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& registry = m_Context->Registry->Raw();

        auto dirLightView = registry.view<Engine::DirectionalLightComponent>();
        auto pointLightView = registry.view<Engine::PointLightComponent>();
        auto spotLightView = registry.view<Engine::SpotLightComponent>();
        auto ambientLightView = registry.view<Engine::AmbientLightComponent>();

        ImGui::Text("Directional: %zu", dirLightView.size());
        ImGui::Text("Point: %zu", pointLightView.size());
        ImGui::Text("Spot: %zu", spotLightView.size());
        ImGui::Text("Ambient: %zu", ambientLightView.size());
    }

    // Rendering stats
    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (m_Context->LightingSystem) {
            auto& stats = m_Context->LightingSystem->GetStats();
            ImGui::Text("Entities Rendered: %u", stats.EntitiesRendered);
        }

        if (m_Context->ShadowSystem) {
            auto& stats = m_Context->ShadowSystem->GetStats();
            ImGui::Text("Shadow Casters: %u", stats.ShadowCastersRendered);
            ImGui::Text("Cascades Rendered: %u", stats.CascadesRendered);
            ImGui::Text("Spot Shadows: %u", stats.SpotShadowsRendered);
        }
    }

    // Viewport info
    if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Size: %.0f x %.0f", m_Context->ViewportSize.x, m_Context->ViewportSize.y);
        ImGui::Text("Focused: %s", m_Context->ViewportFocused ? "Yes" : "No");
        ImGui::Text("Hovered: %s", m_Context->ViewportHovered ? "Yes" : "No");
    }

    // Selection info
    if (ImGui::CollapsingHeader("Selection")) {
        if (m_Context->HasSelection()) {
            ImGui::Text("Selected: Entity %u",
                static_cast<Engine::u32>(m_Context->SelectedEntity));
            ImGui::Text("Multi-select: %zu entities", m_Context->MultiSelection.size());
        } else {
            ImGui::TextDisabled("No selection");
        }
    }

    ImGui::End();
}

} // namespace Editor
