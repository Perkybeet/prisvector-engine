#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"

namespace Demos {

class ShadowQualityDemo : public DemoBase {
public:
    const char* GetName() const override { return "Shadow Quality Demo"; }
    const char* GetDescription() const override {
        return "CSM and spot shadow testing at various distances";
    }

    void OnInit() override {
        LOG_INFO("Initializing Shadow Quality Demo");

        float aspect = GetWindow().GetAspectRatio();

        Engine::FPSCameraSettings fpsSettings;
        fpsSettings.InitialPosition = {0.0f, 5.0f, 30.0f};
        fpsSettings.MovementSpeed = 15.0f;
        auto fpsCamera = Engine::CreateScope<Engine::FPSCameraController>(aspect, fpsSettings);

        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 60.0f;
        orbSettings.Elevation = 40.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        m_CameraManager.Register("fps", std::move(fpsCamera));
        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.SetActive("fps");
        Engine::Input::SetCursorMode(false);

        InitializeRenderingSystems();

        CreateGround();
        CreateShadowCasters();
        CreateLighting();

        m_Exposure = 1.2f;

        glEnable(GL_DEPTH_TEST);
    }

    void OnShutdown() override {
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D1)) {
            m_CameraManager.SetActive("fps");
            Engine::Input::SetCursorMode(false);
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D2)) {
            m_CameraManager.SetActive("orbital");
            Engine::Input::SetCursorMode(true);
        }

        if (Engine::Input::IsKeyJustPressed(Engine::Key::Tab)) {
            m_CursorEnabled = !m_CursorEnabled;
            Engine::Input::SetCursorMode(m_CursorEnabled);
        }

        if (Engine::Input::IsKeyPressed(Engine::Key::Equal)) {
            m_Exposure = glm::min(3.0f, m_Exposure + dt * 1.5f);
        }
        if (Engine::Input::IsKeyPressed(Engine::Key::Minus)) {
            m_Exposure = glm::max(0.1f, m_Exposure - dt * 1.5f);
        }

        m_CameraManager.OnUpdate(dt);
        m_Time += dt;

        // Rotate shadow casters
        for (size_t i = 0; i < m_RotatingCasters.size(); i++) {
            auto& t = m_Registry.get<Engine::Transform>(m_RotatingCasters[i]);
            float angle = m_Time * 0.5f + static_cast<float>(i) * glm::pi<float>() / m_RotatingCasters.size();
            glm::quat rot = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
            t.SetRotation(rot);
            t.UpdateWorldMatrix();
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

        ImGui::Begin("Shadow Quality", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", Engine::Time::GetFPS());

        auto& shadowStats = m_ShadowSystem->GetStats();
        ImGui::Text("Shadow Casters: %u", shadowStats.ShadowCastersRendered);
        ImGui::Text("Cascades: %u", shadowStats.CascadesRendered);
        ImGui::Text("Spot Shadows: %u", shadowStats.SpotShadowsRendered);

        ImGui::Separator();

        auto& settings = m_ShadowSystem->GetSettings();
        ImGui::Checkbox("Shadows Enabled", &settings.Enabled);

        int cascadeRes = static_cast<int>(settings.CascadeResolution);
        const char* resolutions[] = {"512", "1024", "2048", "4096"};
        int resIndex = (cascadeRes == 512) ? 0 : (cascadeRes == 1024) ? 1 : (cascadeRes == 2048) ? 2 : 3;

        if (ImGui::Combo("CSM Resolution", &resIndex, resolutions, 4)) {
            int newRes[] = {512, 1024, 2048, 4096};
            settings.CascadeResolution = static_cast<Engine::u32>(newRes[resIndex]);
        }

        float maxDist = settings.MaxShadowDistance;
        if (ImGui::SliderFloat("Max Distance", &maxDist, 50.0f, 300.0f)) {
            settings.MaxShadowDistance = maxDist;
        }

        ImGui::SliderFloat("Split Lambda", &settings.CascadeSplitLambda, 0.0f, 1.0f);

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);

        ImGui::End();
    }

private:
    void CreateGround() {
        // Very large ground for distance testing
        CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {200.0f, 0.5f, 200.0f},
            {0.3f, 0.4f, 0.3f, 1.0f},  // Green-ish grass
            0.0f, 0.85f
        );

        // Distance markers
        for (int dist = 20; dist <= 180; dist += 20) {
            // Line marker
            CreateObject(m_CubeMesh,
                {0.0f, 0.02f, -static_cast<float>(dist)},
                {10.0f, 0.02f, 0.3f},
                {0.8f, 0.8f, 0.2f, 1.0f},
                0.0f, 0.5f, false
            );
        }
    }

    void CreateShadowCasters() {
        // Near field objects (cascade 0)
        for (int x = -2; x <= 2; x++) {
            auto caster = CreateObject(m_CubeMesh,
                {static_cast<float>(x) * 3.0f, 2.0f, -5.0f},
                {1.5f, 4.0f, 1.5f},
                {0.7f, 0.7f, 0.75f, 1.0f},
                0.1f, 0.6f
            );
            m_RotatingCasters.push_back(caster);
        }

        // Mid-range objects (cascade 1-2)
        for (int dist = 30; dist <= 80; dist += 25) {
            for (int x = -3; x <= 3; x += 2) {
                CreateObject(m_CubeMesh,
                    {static_cast<float>(x) * 5.0f, 3.0f, -static_cast<float>(dist)},
                    {2.0f, 6.0f, 2.0f},
                    {0.6f, 0.6f, 0.65f, 1.0f},
                    0.2f, 0.5f
                );
            }
        }

        // Far objects (cascade 3)
        for (int dist = 100; dist <= 180; dist += 20) {
            CreateObject(m_CubeMesh,
                {0.0f, 5.0f, -static_cast<float>(dist)},
                {4.0f, 10.0f, 4.0f},
                {0.5f, 0.5f, 0.55f, 1.0f},
                0.3f, 0.4f
            );
        }

        // Spheres for soft shadow testing
        for (int z = 10; z <= 50; z += 20) {
            CreateObject(m_SphereMesh,
                {-8.0f, 2.0f, -static_cast<float>(z)},
                {2.5f, 2.5f, 2.5f},
                {0.8f, 0.2f, 0.2f, 1.0f},
                0.0f, 0.4f
            );

            CreateObject(m_SphereMesh,
                {8.0f, 2.0f, -static_cast<float>(z)},
                {2.5f, 2.5f, 2.5f},
                {0.2f, 0.2f, 0.8f, 1.0f},
                0.8f, 0.2f
            );
        }
    }

    void CreateLighting() {
        // Ambient
        CreateAmbientLight({0.15f, 0.18f, 0.25f}, 1.0f);

        // Sun - main shadow caster
        CreateDirectionalLight(
            {0.4f, -0.7f, 0.5f},
            {1.0f, 0.95f, 0.85f},
            1.0f,
            true
        );

        // Spotlights with shadows
        CreateSpotLight(
            {-15.0f, 12.0f, -20.0f},
            glm::normalize(glm::vec3(0.5f, -0.8f, -0.3f)),
            {1.0f, 0.9f, 0.8f},
            35.0f, 20.0f, 30.0f, 40.0f, true
        );

        CreateSpotLight(
            {15.0f, 12.0f, -20.0f},
            glm::normalize(glm::vec3(-0.5f, -0.8f, -0.3f)),
            {0.8f, 0.9f, 1.0f},
            35.0f, 20.0f, 30.0f, 40.0f, true
        );
    }

private:
    Engine::Vector<entt::entity> m_RotatingCasters;
    bool m_CursorEnabled = false;
};

REGISTER_DEMO(ShadowQualityDemo)

} // namespace Demos
