#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"

namespace Demos {

class IndustrialDemo : public DemoBase {
public:
    const char* GetName() const override { return "Industrial Complex"; }
    const char* GetDescription() const override {
        return "Sci-Fi industrial scene with dramatic lighting, 60+ lights, shadows";
    }

    void OnInit() override {
        LOG_INFO("Initializing Industrial Complex Demo");

        float aspect = GetWindow().GetAspectRatio();

        // Setup cameras
        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 45.0f;
        orbSettings.Elevation = 30.0f;
        orbSettings.Azimuth = 45.0f;
        orbSettings.Smoothing = 6.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        Engine::FPSCameraSettings fpsSettings;
        fpsSettings.InitialPosition = {0.0f, 5.0f, 30.0f};
        fpsSettings.MovementSpeed = 12.0f;
        fpsSettings.MouseSensitivity = 0.12f;
        auto fpsCamera = Engine::CreateScope<Engine::FPSCameraController>(aspect, fpsSettings);

        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.Register("fps", std::move(fpsCamera));
        m_CameraManager.SetActive("orbital");
        Engine::Input::SetCursorMode(true);

        InitializeRenderingSystems();

        // Build the industrial scene
        CreateFloor();
        CreateWalls();
        CreateCeiling();
        CreateReactorCore();
        CreateControlConsoles();
        CreateCrates();
        CreatePipes();
        CreateLighting();

        // Configure shadow settings for dramatic effect
        auto& shadowSettings = m_ShadowSystem->GetSettings();
        shadowSettings.MaxShadowDistance = 100.0f;

        m_Exposure = 0.8f;  // Slightly darker for atmosphere

        glEnable(GL_DEPTH_TEST);

        LOG_INFO("Scene created: {} entities, {} lights",
                 m_EntityCount, m_PointLightCount + m_SpotLightCount + 1);
        LOG_INFO("Controls: 1=Orbital, 2=FPS, L=Animate lights, +/-=Exposure");
    }

    void OnShutdown() override {
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        // Camera switching
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D1)) {
            m_CameraManager.SetActive("orbital");
            Engine::Input::SetCursorMode(true);
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D2)) {
            m_CameraManager.SetActive("fps");
            Engine::Input::SetCursorMode(false);
        }

        // Controls
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

        // Animate lights
        if (m_AnimateLights) {
            AnimateLights(dt);
        }

        // Animate warning lights (always on)
        AnimateWarningLights(dt);
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

        ImGui::Begin("Industrial Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        auto& lightStats = m_LightingSystem->GetStats();
        auto& shadowStats = m_ShadowSystem->GetStats();

        ImGui::Text("FPS: %.0f", Engine::Time::GetFPS());
        ImGui::Text("Entities: %u", lightStats.EntitiesRendered);
        ImGui::Text("Lights: D%u P%u S%u",
                    lightStats.DirectionalLightCount,
                    lightStats.PointLightCount,
                    lightStats.SpotLightCount);

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);
        ImGui::Checkbox("Animate Lights", &m_AnimateLights);

        auto& shadowSettings = m_ShadowSystem->GetSettings();
        ImGui::Checkbox("Shadows", &shadowSettings.Enabled);

        ImGui::Separator();

        const char* cameras[] = {"orbital", "fps"};
        int current = (m_CameraManager.GetActiveName() == "orbital") ? 0 : 1;
        if (ImGui::Combo("Camera", &current, cameras, 2)) {
            m_CameraManager.SetActive(cameras[current]);
            Engine::Input::SetCursorMode(current == 0);
        }

        ImGui::End();
    }

private:
    void CreateFloor() {
        // Main floor - dark metal grating
        auto floor = CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {60.0f, 0.5f, 60.0f},
            {0.08f, 0.08f, 0.1f, 1.0f},
            0.9f, 0.6f
        );
        m_EntityCount++;

        // Floor accent lines (simulated with thin strips)
        for (int i = -5; i <= 5; i++) {
            if (i == 0) continue;
            auto line = CreateObject(m_CubeMesh,
                {static_cast<float>(i) * 5.0f, 0.01f, 0.0f},
                {0.1f, 0.01f, 60.0f},
                {0.2f, 0.6f, 0.8f, 1.0f},
                0.0f, 0.3f
            );
            m_EntityCount++;
        }
    }

    void CreateWalls() {
        float wallHeight = 12.0f;
        float wallThickness = 0.5f;
        float roomSize = 30.0f;

        // Dark industrial wall color
        glm::vec4 wallColor = {0.06f, 0.06f, 0.08f, 1.0f};

        // Back wall
        CreateObject(m_CubeMesh,
            {0.0f, wallHeight * 0.5f, -roomSize},
            {roomSize * 2.0f, wallHeight, wallThickness},
            wallColor, 0.0f, 0.85f
        );
        m_EntityCount++;

        // Left wall
        CreateObject(m_CubeMesh,
            {-roomSize, wallHeight * 0.5f, 0.0f},
            {wallThickness, wallHeight, roomSize * 2.0f},
            wallColor, 0.0f, 0.85f
        );
        m_EntityCount++;

        // Right wall
        CreateObject(m_CubeMesh,
            {roomSize, wallHeight * 0.5f, 0.0f},
            {wallThickness, wallHeight, roomSize * 2.0f},
            wallColor, 0.0f, 0.85f
        );
        m_EntityCount++;

        // Wall accent strips (cyan neon trim)
        for (int z = -25; z <= 25; z += 10) {
            // Left wall strips
            CreateObject(m_CubeMesh,
                {-roomSize + 0.3f, 2.0f, static_cast<float>(z)},
                {0.05f, 0.1f, 8.0f},
                {0.0f, 0.8f, 1.0f, 1.0f},
                0.0f, 0.1f
            );
            m_EntityCount++;

            // Right wall strips
            CreateObject(m_CubeMesh,
                {roomSize - 0.3f, 2.0f, static_cast<float>(z)},
                {0.05f, 0.1f, 8.0f},
                {0.0f, 0.8f, 1.0f, 1.0f},
                0.0f, 0.1f
            );
            m_EntityCount++;
        }
    }

    void CreateCeiling() {
        float ceilingHeight = 12.0f;

        // Main ceiling panels
        CreateObject(m_CubeMesh,
            {0.0f, ceilingHeight, 0.0f},
            {60.0f, 0.3f, 60.0f},
            {0.04f, 0.04f, 0.05f, 1.0f},
            0.0f, 0.9f
        );
        m_EntityCount++;

        // Support beams
        for (int x = -20; x <= 20; x += 10) {
            CreateObject(m_CubeMesh,
                {static_cast<float>(x), ceilingHeight - 1.0f, 0.0f},
                {0.5f, 2.0f, 60.0f},
                {0.15f, 0.15f, 0.18f, 1.0f},
                0.8f, 0.4f
            );
            m_EntityCount++;
        }
    }

    void CreateReactorCore() {
        // Central reactor structure
        glm::vec3 reactorPos = {0.0f, 0.0f, 0.0f};

        // Base platform
        CreateObject(m_CubeMesh,
            reactorPos + glm::vec3(0.0f, 0.5f, 0.0f),
            {8.0f, 1.0f, 8.0f},
            {0.12f, 0.12f, 0.15f, 1.0f},
            0.9f, 0.3f
        );
        m_EntityCount++;

        // Core cylinder (approximated with cube for now)
        CreateObject(m_CubeMesh,
            reactorPos + glm::vec3(0.0f, 4.0f, 0.0f),
            {3.0f, 6.0f, 3.0f},
            {0.8f, 0.85f, 0.9f, 1.0f},
            1.0f, 0.1f  // Chrome finish
        );
        m_EntityCount++;

        // Glowing core rings
        for (int i = 0; i < 3; i++) {
            float y = 2.0f + i * 2.0f;
            CreateObject(m_CubeMesh,
                reactorPos + glm::vec3(0.0f, y, 0.0f),
                {4.0f, 0.3f, 4.0f},
                {0.0f, 0.8f, 1.0f, 1.0f},  // Cyan glow
                0.0f, 0.1f
            );
            m_EntityCount++;
        }

        // Reactor pillars
        for (int i = 0; i < 4; i++) {
            float angle = static_cast<float>(i) * glm::half_pi<float>();
            float x = glm::cos(angle) * 5.0f;
            float z = glm::sin(angle) * 5.0f;

            CreateObject(m_CubeMesh,
                reactorPos + glm::vec3(x, 3.0f, z),
                {0.8f, 6.0f, 0.8f},
                {0.2f, 0.2f, 0.25f, 1.0f},
                0.7f, 0.5f
            );
            m_EntityCount++;
        }
    }

    void CreateControlConsoles() {
        // Control room consoles
        struct ConsoleDef {
            glm::vec3 pos;
            float rotation;
        };

        ConsoleDef consoles[] = {
            {{-15.0f, 0.0f, -20.0f}, 0.0f},
            {{-10.0f, 0.0f, -20.0f}, 0.0f},
            {{-5.0f, 0.0f, -20.0f}, 0.0f},
            {{5.0f, 0.0f, -20.0f}, 0.0f},
            {{10.0f, 0.0f, -20.0f}, 0.0f},
            {{15.0f, 0.0f, -20.0f}, 0.0f},
        };

        for (const auto& def : consoles) {
            // Console base
            CreateObject(m_CubeMesh,
                def.pos + glm::vec3(0.0f, 0.6f, 0.0f),
                {2.0f, 1.2f, 1.0f},
                {0.1f, 0.1f, 0.12f, 1.0f},
                0.5f, 0.6f
            );
            m_EntityCount++;

            // Console screen (angled)
            CreateObject(m_CubeMesh,
                def.pos + glm::vec3(0.0f, 1.4f, -0.3f),
                {1.8f, 0.8f, 0.1f},
                {0.1f, 0.3f, 0.15f, 1.0f},  // Green tint
                0.0f, 0.2f
            );
            m_EntityCount++;
        }
    }

    void CreateCrates() {
        // Storage area crates
        glm::vec4 crateColors[] = {
            {0.4f, 0.35f, 0.2f, 1.0f},   // Yellow-ish
            {0.3f, 0.15f, 0.1f, 1.0f},   // Red-brown
            {0.15f, 0.2f, 0.25f, 1.0f},  // Blue-gray
        };

        // Left storage area
        for (int row = 0; row < 3; row++) {
            for (int stack = 0; stack < 2; stack++) {
                float x = -25.0f + row * 3.0f;
                float y = 1.0f + stack * 2.0f;
                float z = 15.0f + (row % 2) * 2.0f;

                glm::vec4 color = crateColors[(row + stack) % 3];
                CreateObject(m_CubeMesh,
                    {x, y, z},
                    {2.0f, 2.0f, 2.0f},
                    color, 0.0f, 0.75f
                );
                m_EntityCount++;
            }
        }

        // Right storage area
        for (int row = 0; row < 4; row++) {
            for (int stack = 0; stack < 3 - row; stack++) {
                float x = 22.0f + row * 2.5f;
                float y = 1.0f + stack * 2.0f;
                float z = -15.0f;

                glm::vec4 color = crateColors[(row + stack + 1) % 3];
                CreateObject(m_CubeMesh,
                    {x, y, z},
                    {2.0f, 2.0f, 2.0f},
                    color, 0.0f, 0.75f
                );
                m_EntityCount++;
            }
        }
    }

    void CreatePipes() {
        // Ceiling pipes (using cylinders rotated)
        float pipeRadius = 0.3f;
        glm::vec4 pipeColor = {0.5f, 0.5f, 0.55f, 1.0f};

        // Horizontal pipes along ceiling
        for (int i = 0; i < 4; i++) {
            float z = -20.0f + i * 13.0f;
            CreateObject(m_CubeMesh,  // Using cube for now, cylinder better
                {0.0f, 10.5f, z},
                {58.0f, 0.6f, 0.6f},
                pipeColor, 0.9f, 0.3f
            );
            m_EntityCount++;
        }

        // Vertical pipes along walls
        for (int i = 0; i < 6; i++) {
            float z = -25.0f + i * 10.0f;

            // Left wall pipes
            CreateObject(m_CubeMesh,
                {-28.0f, 5.0f, z},
                {0.5f, 10.0f, 0.5f},
                pipeColor, 0.9f, 0.3f
            );
            m_EntityCount++;

            // Right wall pipes
            CreateObject(m_CubeMesh,
                {28.0f, 5.0f, z},
                {0.5f, 10.0f, 0.5f},
                pipeColor, 0.9f, 0.3f
            );
            m_EntityCount++;
        }
    }

    void CreateLighting() {
        // Ambient - very dark for dramatic effect
        CreateAmbientLight({0.02f, 0.02f, 0.04f}, 1.0f);

        // Main directional light (dim overhead emergency lighting)
        CreateDirectionalLight(
            {0.2f, -1.0f, 0.1f},
            {0.3f, 0.35f, 0.4f},
            0.3f,
            true
        );

        // === CEILING POINT LIGHTS (Industrial overhead) ===
        // Grid of ceiling lights
        for (int x = -20; x <= 20; x += 8) {
            for (int z = -20; z <= 20; z += 8) {
                // Alternate between cool white and tech blue
                bool isTechBlue = ((x + z) / 8) % 3 == 0;
                glm::vec3 color = isTechBlue
                    ? glm::vec3(0.4f, 0.6f, 1.0f)   // Tech blue
                    : glm::vec3(0.9f, 0.95f, 1.0f);  // Cool white

                float intensity = isTechBlue ? 8.0f : 12.0f;

                auto light = CreatePointLight(
                    {static_cast<float>(x), 11.0f, static_cast<float>(z)},
                    color, intensity, 15.0f
                );
                m_CeilingLights.push_back(light);
                m_PointLightCount++;
            }
        }

        // === REACTOR CORE LIGHTS ===
        // Bright cyan point lights around reactor
        for (int i = 0; i < 8; i++) {
            float angle = static_cast<float>(i) * glm::quarter_pi<float>();
            float x = glm::cos(angle) * 6.0f;
            float z = glm::sin(angle) * 6.0f;

            auto light = CreatePointLight(
                {x, 3.0f, z},
                {0.0f, 0.9f, 1.0f},  // Cyan
                15.0f, 12.0f
            );
            m_ReactorLights.push_back(light);
            m_PointLightCount++;
        }

        // Central reactor glow
        auto coreLight = CreatePointLight(
            {0.0f, 4.0f, 0.0f},
            {0.2f, 0.8f, 1.0f},
            25.0f, 20.0f
        );
        m_ReactorLights.push_back(coreLight);
        m_PointLightCount++;

        // === WALL ACCENT LIGHTS ===
        // Blue accent strips along walls
        for (int z = -20; z <= 20; z += 10) {
            // Left wall
            auto leftLight = CreatePointLight(
                {-29.0f, 2.0f, static_cast<float>(z)},
                {0.0f, 0.5f, 1.0f},
                6.0f, 8.0f
            );
            m_AccentLights.push_back(leftLight);
            m_PointLightCount++;

            // Right wall
            auto rightLight = CreatePointLight(
                {29.0f, 2.0f, static_cast<float>(z)},
                {0.0f, 0.5f, 1.0f},
                6.0f, 8.0f
            );
            m_AccentLights.push_back(rightLight);
            m_PointLightCount++;
        }

        // === SPOTLIGHTS (with shadows) ===
        // Security spotlights pointing at key areas

        // Reactor spotlights
        CreateSpotLight(
            {15.0f, 10.0f, 15.0f},
            glm::normalize(glm::vec3(-1.0f, -0.8f, -1.0f)),
            {1.0f, 0.95f, 0.9f},
            30.0f, 15.0f, 25.0f, 35.0f, true
        );
        m_SpotLightCount++;

        CreateSpotLight(
            {-15.0f, 10.0f, 15.0f},
            glm::normalize(glm::vec3(1.0f, -0.8f, -1.0f)),
            {1.0f, 0.95f, 0.9f},
            30.0f, 15.0f, 25.0f, 35.0f, true
        );
        m_SpotLightCount++;

        // Control room spotlights
        CreateSpotLight(
            {0.0f, 10.0f, -15.0f},
            glm::normalize(glm::vec3(0.0f, -1.0f, -0.3f)),
            {0.95f, 0.98f, 1.0f},
            25.0f, 20.0f, 30.0f, 30.0f, true
        );
        m_SpotLightCount++;

        // Storage area spotlights
        CreateSpotLight(
            {-25.0f, 10.0f, 10.0f},
            glm::normalize(glm::vec3(0.0f, -1.0f, 0.2f)),
            {1.0f, 0.9f, 0.7f},  // Warm
            20.0f, 18.0f, 28.0f, 25.0f, true
        );
        m_SpotLightCount++;

        CreateSpotLight(
            {25.0f, 10.0f, -10.0f},
            glm::normalize(glm::vec3(-0.1f, -1.0f, 0.1f)),
            {1.0f, 0.9f, 0.7f},
            20.0f, 18.0f, 28.0f, 25.0f, true
        );
        m_SpotLightCount++;

        // === WARNING LIGHTS ===
        // Orange warning lights that pulse
        auto warning1 = CreatePointLight(
            {-28.0f, 8.0f, 25.0f},
            {1.0f, 0.4f, 0.0f},
            10.0f, 12.0f
        );
        m_WarningLights.push_back(warning1);
        m_PointLightCount++;

        auto warning2 = CreatePointLight(
            {28.0f, 8.0f, 25.0f},
            {1.0f, 0.4f, 0.0f},
            10.0f, 12.0f
        );
        m_WarningLights.push_back(warning2);
        m_PointLightCount++;

        auto warning3 = CreatePointLight(
            {0.0f, 11.0f, -28.0f},
            {1.0f, 0.4f, 0.0f},
            10.0f, 12.0f
        );
        m_WarningLights.push_back(warning3);
        m_PointLightCount++;
    }

    void AnimateLights(Engine::f32 dt) {
        // Subtle reactor light pulsing
        for (size_t i = 0; i < m_ReactorLights.size(); i++) {
            auto& light = m_Registry.get<Engine::PointLightComponent>(m_ReactorLights[i]);
            float phase = m_Time * 2.0f + static_cast<float>(i) * 0.5f;
            float pulse = 0.85f + 0.15f * glm::sin(phase);
            light.Intensity = (i == m_ReactorLights.size() - 1) ? 25.0f * pulse : 15.0f * pulse;
        }

        // Slight flicker on some ceiling lights
        for (size_t i = 0; i < m_CeilingLights.size(); i += 7) {
            auto& light = m_Registry.get<Engine::PointLightComponent>(m_CeilingLights[i]);
            float flicker = 0.9f + 0.1f * glm::sin(m_Time * 15.0f + i * 3.14f);
            light.Intensity = 12.0f * flicker;
        }
    }

    void AnimateWarningLights(Engine::f32 dt) {
        // Warning lights pulse slowly
        float pulse = 0.5f + 0.5f * glm::sin(m_Time * 2.0f);

        for (auto& warningEntity : m_WarningLights) {
            auto& light = m_Registry.get<Engine::PointLightComponent>(warningEntity);
            light.Intensity = 5.0f + 15.0f * pulse;
        }
    }

private:
    Engine::u32 m_EntityCount = 0;
    Engine::u32 m_PointLightCount = 0;
    Engine::u32 m_SpotLightCount = 0;

    Engine::Vector<entt::entity> m_CeilingLights;
    Engine::Vector<entt::entity> m_ReactorLights;
    Engine::Vector<entt::entity> m_AccentLights;
    Engine::Vector<entt::entity> m_WarningLights;

    bool m_AnimateLights = true;
};

REGISTER_DEMO(IndustrialDemo)

} // namespace Demos
