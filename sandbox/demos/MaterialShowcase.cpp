#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"

namespace Demos {

class MaterialShowcase : public DemoBase {
public:
    const char* GetName() const override { return "Material Showcase"; }
    const char* GetDescription() const override {
        return "PBR material gallery with metallic/roughness variations";
    }

    void OnInit() override {
        LOG_INFO("Initializing Material Showcase");

        float aspect = GetWindow().GetAspectRatio();

        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 25.0f;
        orbSettings.Elevation = 30.0f;
        orbSettings.Smoothing = 8.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.SetActive("orbital");
        Engine::Input::SetCursorMode(true);

        InitializeRenderingSystems();

        CreateEnvironment();
        CreateMaterialGrid();
        CreateLighting();

        m_Exposure = 1.0f;

        glEnable(GL_DEPTH_TEST);
    }

    void OnShutdown() override {
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        // Material set switching
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D1)) {
            m_CurrentMaterialSet = 0;
            UpdateMaterials();
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D2)) {
            m_CurrentMaterialSet = 1;
            UpdateMaterials();
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D3)) {
            m_CurrentMaterialSet = 2;
            UpdateMaterials();
        }

        if (Engine::Input::IsKeyJustPressed(Engine::Key::L)) {
            m_AnimateLights = !m_AnimateLights;
        }

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

        ImGui::Begin("Material Showcase", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", Engine::Time::GetFPS());

        ImGui::Separator();

        const char* materialSets[] = {
            "Gold/Copper/Chrome",
            "Plastic Colors",
            "Raw Materials"
        };

        if (ImGui::Combo("Material Set", &m_CurrentMaterialSet, materialSets, 3)) {
            UpdateMaterials();
        }

        ImGui::Separator();

        ImGui::Text("Rows: Roughness (0 to 1)");
        ImGui::Text("Cols: Metallic (0 to 1)");

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);
        ImGui::Checkbox("Animate Lights", &m_AnimateLights);

        ImGui::Separator();
        ImGui::TextDisabled("1/2/3: Switch material sets");

        ImGui::End();
    }

private:
    void CreateEnvironment() {
        // Dark backdrop
        CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {40.0f, 0.5f, 40.0f},
            {0.05f, 0.05f, 0.06f, 1.0f},
            0.0f, 0.9f, false
        );

        // Back wall
        CreateObject(m_CubeMesh,
            {0.0f, 5.0f, -15.0f},
            {40.0f, 12.0f, 0.5f},
            {0.04f, 0.04f, 0.05f, 1.0f},
            0.0f, 0.95f, false
        );
    }

    void CreateMaterialGrid() {
        // 10x10 grid of spheres
        m_GridSize = 10;
        float spacing = 2.2f;
        float offset = (m_GridSize - 1) * spacing * 0.5f;

        m_Spheres.reserve(m_GridSize * m_GridSize);

        for (int row = 0; row < m_GridSize; row++) {
            for (int col = 0; col < m_GridSize; col++) {
                float x = col * spacing - offset;
                float z = row * spacing - offset;

                auto sphere = CreateObject(m_SphereMesh,
                    {x, 1.2f, z},
                    {1.0f, 1.0f, 1.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},  // Will be updated
                    0.0f, 0.5f
                );

                m_Spheres.push_back(sphere);
            }
        }

        UpdateMaterials();
    }

    void UpdateMaterials() {
        for (int row = 0; row < m_GridSize; row++) {
            for (int col = 0; col < m_GridSize; col++) {
                int idx = row * m_GridSize + col;
                auto& mat = m_Registry.get<Engine::MaterialComponent>(m_Spheres[idx]);

                float metallic = static_cast<float>(col) / (m_GridSize - 1);
                float roughness = static_cast<float>(row) / (m_GridSize - 1);
                roughness = glm::max(0.05f, roughness);

                glm::vec3 baseColor;

                switch (m_CurrentMaterialSet) {
                    case 0: // Metals
                        baseColor = glm::mix(
                            glm::vec3(1.0f, 0.85f, 0.57f),  // Gold
                            glm::vec3(0.95f, 0.93f, 0.88f), // Chrome
                            metallic
                        );
                        break;

                    case 1: // Plastics
                        {
                            float hue = static_cast<float>(col) / m_GridSize;
                            baseColor = HueToRGB(hue);
                            metallic = 0.0f;  // Override to non-metallic
                        }
                        break;

                    case 2: // Raw materials
                        {
                            // Transition from concrete to rusted metal
                            glm::vec3 concrete = {0.5f, 0.5f, 0.5f};
                            glm::vec3 rust = {0.6f, 0.35f, 0.2f};
                            glm::vec3 copper = {0.95f, 0.64f, 0.54f};
                            baseColor = glm::mix(
                                glm::mix(concrete, rust, metallic * 0.7f),
                                copper,
                                metallic * metallic
                            );
                        }
                        break;
                }

                mat.BaseColor = glm::vec4(baseColor, 1.0f);
                mat.Metallic = metallic;
                mat.Roughness = roughness;
            }
        }
    }

    void CreateLighting() {
        // Soft ambient
        CreateAmbientLight({0.02f, 0.02f, 0.025f}, 1.0f);

        // Key light - warm
        CreateDirectionalLight(
            {0.5f, -0.8f, 0.3f},
            {1.0f, 0.95f, 0.9f},
            0.6f,
            true
        );

        // Fill light - cool
        CreateDirectionalLight(
            {-0.4f, -0.5f, -0.2f},
            {0.6f, 0.7f, 1.0f},
            0.3f,
            false
        );

        // Rim light - back
        CreateDirectionalLight(
            {0.0f, -0.3f, -1.0f},
            {0.9f, 0.9f, 1.0f},
            0.4f,
            false
        );

        // Accent point lights
        glm::vec3 positions[] = {
            {-10.0f, 4.0f, 5.0f},
            {10.0f, 4.0f, 5.0f},
            {-10.0f, 4.0f, -5.0f},
            {10.0f, 4.0f, -5.0f},
        };

        glm::vec3 colors[] = {
            {1.0f, 0.8f, 0.6f},
            {0.6f, 0.8f, 1.0f},
            {0.8f, 1.0f, 0.8f},
            {1.0f, 0.6f, 0.8f},
        };

        for (int i = 0; i < 4; i++) {
            auto light = CreatePointLight(positions[i], colors[i], 15.0f, 20.0f);
            m_AccentLights.push_back(light);
        }
    }

    void AnimateLights() {
        for (size_t i = 0; i < m_AccentLights.size(); i++) {
            auto& t = m_Registry.get<Engine::Transform>(m_AccentLights[i]);
            float angle = m_Time * 0.3f + static_cast<float>(i) * glm::half_pi<float>();
            float radius = 12.0f;

            t.SetPosition(
                glm::cos(angle) * radius,
                4.0f + glm::sin(m_Time * 0.5f + i) * 1.5f,
                glm::sin(angle) * radius
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
    int m_GridSize = 10;
    int m_CurrentMaterialSet = 0;
    Engine::Vector<entt::entity> m_Spheres;
    Engine::Vector<entt::entity> m_AccentLights;
    bool m_AnimateLights = true;
};

REGISTER_DEMO(MaterialShowcase)

} // namespace Demos
