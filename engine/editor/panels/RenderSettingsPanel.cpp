#include "RenderSettingsPanel.hpp"
#include <imgui.h>

namespace Editor {

void RenderSettingsPanel::OnImGuiRender() {
    ImGui::Begin("Render Settings");

    // Post-Processing
    if (ImGui::CollapsingHeader("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Exposure", &m_Context->Exposure, 0.1f, 10.0f);
        ImGui::SliderFloat("Gamma", &m_Context->Gamma, 1.0f, 3.0f);

        if (ImGui::Button("Reset Post-Process")) {
            m_Context->Exposure = 1.0f;
            m_Context->Gamma = 2.2f;
        }
    }

    // Shadow Settings
    if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Shadows", &m_Context->ShadowsEnabled);

        if (m_Context->ShadowsEnabled && m_Context->ShadowSystem) {
            auto& settings = m_Context->ShadowSystem->GetSettings();

            // CSM Settings
            ImGui::Text("Cascaded Shadow Maps:");
            ImGui::Indent();

            static const char* resolutions[] = {"512", "1024", "2048", "4096"};
            static int currentRes = 2; // Default 2048
            if (ImGui::Combo("Resolution", &currentRes, resolutions, 4)) {
                int res = 512 << currentRes;
                settings.CascadeResolution = res;
            }

            ImGui::SliderFloat("Max Distance", &settings.MaxShadowDistance, 10.0f, 500.0f);
            ImGui::SliderFloat("Cascade Lambda", &settings.CascadeSplitLambda, 0.0f, 1.0f);

            ImGui::Unindent();

            // Shadow quality
            ImGui::Separator();
            ImGui::Text("Quality:");
            ImGui::Indent();

            ImGui::SliderInt("PCF Samples", reinterpret_cast<int*>(&settings.PCFSamples), 1, 64);
            ImGui::Checkbox("Use PCSS", &settings.UsePCSS);

            ImGui::Unindent();
        }
    }

    // Debug Views
    if (ImGui::CollapsingHeader("Debug Views", ImGuiTreeNodeFlags_DefaultOpen)) {
        static const char* viewNames[] = {
            "None",
            "CSM Cascades",
            "GBuffer Albedo",
            "GBuffer Normals",
            "GBuffer Depth",
            "GBuffer Metal/Rough"
        };

        int currentView = static_cast<int>(m_Context->CurrentDebugView);
        if (ImGui::Combo("Debug View", &currentView, viewNames, 6)) {
            m_Context->CurrentDebugView = static_cast<Engine::DebugView>(currentView);
            if (m_Context->DebugRenderer) {
                m_Context->DebugRenderer->SetActiveView(m_Context->CurrentDebugView);
            }
        }

        ImGui::TextDisabled("Press F3 to cycle views");
    }

    // Rendering Options
    if (ImGui::CollapsingHeader("Options")) {
        ImGui::Checkbox("VSync", &m_Context->VSyncEnabled);
        ImGui::Checkbox("Wireframe Mode", &m_Context->WireframeMode);
    }

    ImGui::End();
}

} // namespace Editor
