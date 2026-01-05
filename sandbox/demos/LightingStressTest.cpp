#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"

namespace Demos {

class LightingStressTest : public DemoBase {
public:
    const char* GetName() const override { return "Lighting Stress Test"; }
    const char* GetDescription() const override {
        return "Performance test with 100+ dynamic point lights";
    }

    void OnInit() override {
        LOG_INFO("Initializing Lighting Stress Test");

        float aspect = GetWindow().GetAspectRatio();

        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 50.0f;
        orbSettings.Elevation = 35.0f;
        orbSettings.Smoothing = 8.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.SetActive("orbital");
        Engine::Input::SetCursorMode(true);

        InitializeRenderingSystems();

        // Create simple scene
        CreateGround();
        CreatePillars();
        CreateLights();

        m_Exposure = 0.6f;

        glEnable(GL_DEPTH_TEST);

        LOG_INFO("Stress test ready: {} lights (adjust with +/-)", m_ActiveLightCount);
    }

    void OnShutdown() override {
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        // Light count adjustment
        if (Engine::Input::IsKeyJustPressed(Engine::Key::Up)) {
            m_ActiveLightCount = glm::min(256u, m_ActiveLightCount + 16u);
            UpdateActiveLights();
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::Down)) {
            m_ActiveLightCount = glm::max(16u, m_ActiveLightCount - 16u);
            UpdateActiveLights();
        }

        // Animation toggle
        if (Engine::Input::IsKeyJustPressed(Engine::Key::Space)) {
            m_AnimateLights = !m_AnimateLights;
        }

        // Exposure
        if (Engine::Input::IsKeyPressed(Engine::Key::Equal)) {
            m_Exposure = glm::min(3.0f, m_Exposure + dt * 1.5f);
        }
        if (Engine::Input::IsKeyPressed(Engine::Key::Minus)) {
            m_Exposure = glm::max(0.1f, m_Exposure - dt * 1.5f);
        }

        m_CameraManager.OnUpdate(dt);
        m_Time += dt;

        if (m_AnimateLights) {
            AnimateLights();
        }
    }

    void OnEvent(Engine::Event& e) override {
        m_CameraManager.OnEvent(e);
    }

    void OnResize(Engine::u32 width, Engine::u32 height) override {
        m_LightingSystem->Resize(width, height);
    }

    void OnRender() override {
        RenderScene();
        RenderTonemapped();
    }

    void OnImGuiRender() override {
        ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_FirstUseEver);

        ImGui::Begin("Lighting Stress Test", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f (%.2f ms)", Engine::Time::GetFPS(), Engine::Time::GetDeltaTime() * 1000.0f);

        auto& stats = m_LightingSystem->GetStats();
        ImGui::Text("Entities: %u", stats.EntitiesRendered);
        ImGui::Text("Point Lights: %u / %u", stats.PointLightCount, m_TotalLights);

        ImGui::Separator();

        ImGui::Text("Active Lights: %u", m_ActiveLightCount);
        if (ImGui::Button("-16")) {
            m_ActiveLightCount = glm::max(16u, m_ActiveLightCount - 16u);
            UpdateActiveLights();
        }
        ImGui::SameLine();
        if (ImGui::Button("+16")) {
            m_ActiveLightCount = glm::min(256u, m_ActiveLightCount + 16u);
            UpdateActiveLights();
        }

        int count = static_cast<int>(m_ActiveLightCount);
        if (ImGui::SliderInt("Lights", &count, 16, 256)) {
            m_ActiveLightCount = static_cast<Engine::u32>(count);
            UpdateActiveLights();
        }

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);
        ImGui::Checkbox("Animate Lights", &m_AnimateLights);

        ImGui::Separator();
        ImGui::TextDisabled("Up/Down: Adjust light count");
        ImGui::TextDisabled("Space: Toggle animation");

        ImGui::End();
    }

private:
    void CreateGround() {
        // Large reflective floor
        CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {80.0f, 0.5f, 80.0f},
            {0.15f, 0.15f, 0.18f, 1.0f},
            0.2f, 0.4f
        );

        // Grid pattern for visual reference
        for (int i = -7; i <= 7; i++) {
            // X lines
            CreateObject(m_CubeMesh,
                {static_cast<float>(i) * 5.0f, 0.01f, 0.0f},
                {0.05f, 0.01f, 70.0f},
                {0.3f, 0.3f, 0.35f, 1.0f},
                0.0f, 0.5f
            );
            // Z lines
            CreateObject(m_CubeMesh,
                {0.0f, 0.01f, static_cast<float>(i) * 5.0f},
                {70.0f, 0.01f, 0.05f},
                {0.3f, 0.3f, 0.35f, 1.0f},
                0.0f, 0.5f
            );
        }
    }

    void CreatePillars() {
        // Create some objects to receive lighting
        for (int x = -3; x <= 3; x++) {
            for (int z = -3; z <= 3; z++) {
                if (x == 0 && z == 0) continue;

                float height = 2.0f + glm::abs(static_cast<float>(x * z)) * 0.3f;

                CreateObject(m_CubeMesh,
                    {static_cast<float>(x) * 8.0f, height * 0.5f, static_cast<float>(z) * 8.0f},
                    {1.5f, height, 1.5f},
                    {0.7f, 0.7f, 0.75f, 1.0f},
                    0.1f, 0.6f
                );
            }
        }

        // Spheres for specular highlights
        for (int i = 0; i < 12; i++) {
            float angle = static_cast<float>(i) * glm::two_pi<float>() / 12.0f;
            float x = glm::cos(angle) * 20.0f;
            float z = glm::sin(angle) * 20.0f;

            CreateObject(m_SphereMesh,
                {x, 1.5f, z},
                {2.0f, 2.0f, 2.0f},
                {0.9f, 0.9f, 0.95f, 1.0f},
                0.9f, 0.2f  // Metallic
            );
        }
    }

    void CreateLights() {
        // Ambient
        CreateAmbientLight({0.02f, 0.02f, 0.03f}, 1.0f);

        // Single directional for base lighting
        CreateDirectionalLight(
            {0.3f, -1.0f, 0.2f},
            {0.5f, 0.5f, 0.55f},
            0.2f,
            true
        );

        // Generate rainbow of point lights
        m_TotalLights = 256;
        m_LightEntities.reserve(m_TotalLights);
        m_LightBasePositions.reserve(m_TotalLights);

        for (Engine::u32 i = 0; i < m_TotalLights; i++) {
            // Distribute in expanding spiral pattern
            float t = static_cast<float>(i) / m_TotalLights;
            float angle = t * glm::two_pi<float>() * 8.0f;
            float radius = 5.0f + t * 30.0f;

            glm::vec3 pos = {
                glm::cos(angle) * radius,
                2.0f + glm::sin(t * glm::pi<float>() * 4.0f) * 2.0f,
                glm::sin(angle) * radius
            };

            // Rainbow color based on position
            float hue = t;
            glm::vec3 color = HueToRGB(hue);

            auto light = CreatePointLight(pos, color, 8.0f, 12.0f);

            m_LightEntities.push_back(light);
            m_LightBasePositions.push_back(pos);
        }

        m_ActiveLightCount = 128;  // Start with half
        UpdateActiveLights();
    }

    void UpdateActiveLights() {
        for (Engine::u32 i = 0; i < m_TotalLights; i++) {
            auto& light = m_Registry.get<Engine::PointLightComponent>(m_LightEntities[i]);
            light.Enabled = (i < m_ActiveLightCount);
        }
    }

    void AnimateLights() {
        for (Engine::u32 i = 0; i < m_ActiveLightCount && i < m_LightEntities.size(); i++) {
            auto& t = m_Registry.get<Engine::Transform>(m_LightEntities[i]);
            const auto& basePos = m_LightBasePositions[i];

            float phase = m_Time * 0.5f + static_cast<float>(i) * 0.1f;
            float yOffset = glm::sin(phase) * 1.5f;
            float radiusOffset = glm::sin(phase * 0.7f) * 2.0f;

            float currentRadius = glm::length(glm::vec2(basePos.x, basePos.z));
            float newRadius = currentRadius + radiusOffset;
            float angle = glm::atan(basePos.z, basePos.x) + m_Time * 0.1f;

            t.SetPosition(
                glm::cos(angle) * newRadius,
                basePos.y + yOffset,
                glm::sin(angle) * newRadius
            );
            t.UpdateWorldMatrix();
        }
    }

    glm::vec3 HueToRGB(float hue) {
        float r = glm::abs(hue * 6.0f - 3.0f) - 1.0f;
        float g = 2.0f - glm::abs(hue * 6.0f - 2.0f);
        float b = 2.0f - glm::abs(hue * 6.0f - 4.0f);
        return glm::clamp(glm::vec3(r, g, b), 0.0f, 1.0f);
    }

private:
    Engine::u32 m_TotalLights = 0;
    Engine::u32 m_ActiveLightCount = 128;
    Engine::Vector<entt::entity> m_LightEntities;
    Engine::Vector<glm::vec3> m_LightBasePositions;
    bool m_AnimateLights = true;
};

REGISTER_DEMO(LightingStressTest)

} // namespace Demos
