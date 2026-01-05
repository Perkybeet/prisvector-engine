#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"

namespace Demos {

class CameraSystemsDemo : public DemoBase {
public:
    const char* GetName() const override { return "Camera Systems"; }
    const char* GetDescription() const override {
        return "FPS, Orbital, and Third-Person camera demonstrations";
    }

    void OnInit() override {
        LOG_INFO("Initializing Camera Systems Demo");

        float aspect = GetWindow().GetAspectRatio();

        // FPS Camera
        Engine::FPSCameraSettings fpsSettings;
        fpsSettings.InitialPosition = {0.0f, 2.0f, 10.0f};
        fpsSettings.MovementSpeed = 10.0f;
        fpsSettings.MouseSensitivity = 0.12f;
        auto fpsCamera = Engine::CreateScope<Engine::FPSCameraController>(aspect, fpsSettings);

        // Orbital Camera
        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 20.0f;
        orbSettings.Elevation = 25.0f;
        orbSettings.Smoothing = 8.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        // Third-Person Camera
        Engine::ThirdPersonCameraSettings tpSettings;
        tpSettings.Distance = 8.0f;
        tpSettings.HeightOffset = 3.0f;
        tpSettings.Smoothing = 6.0f;
        auto tpCamera = Engine::CreateScope<Engine::ThirdPersonCameraController>(aspect, tpSettings);

        m_CameraManager.Register("fps", std::move(fpsCamera));
        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.Register("third_person", std::move(tpCamera));

        m_CameraManager.SetActive("orbital");
        Engine::Input::SetCursorMode(true);

        InitializeRenderingSystems();

        CreateEnvironment();
        CreateCharacter();
        CreateLighting();

        m_Exposure = 1.0f;

        glEnable(GL_DEPTH_TEST);

        LOG_INFO("Camera Demo: 1=FPS, 2=Orbital, 3=Third-Person");
    }

    void OnShutdown() override {
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        // Camera switching
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D1)) {
            m_CameraManager.SetActive("fps");
            Engine::Input::SetCursorMode(false);
            LOG_INFO("FPS Camera - WASD to move, mouse to look");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D2)) {
            m_CameraManager.SetActive("orbital");
            Engine::Input::SetCursorMode(true);
            LOG_INFO("Orbital Camera - Drag to orbit, scroll to zoom");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D3)) {
            m_CameraManager.SetActive("third_person");
            Engine::Input::SetCursorMode(true);
            UpdateThirdPersonTarget();
            LOG_INFO("Third-Person Camera - Follows character");
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

        m_Time += dt;

        // Animate character movement
        AnimateCharacter(dt);

        // Update third-person target
        if (m_CameraManager.GetActiveName() == "third_person") {
            UpdateThirdPersonTarget();
        }

        m_CameraManager.OnUpdate(dt);
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

        ImGui::Begin("Camera Systems", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", Engine::Time::GetFPS());

        ImGui::Separator();

        ImGui::Text("Active Camera: %s", m_CameraManager.GetActiveName().c_str());

        const char* cameras[] = {"fps", "orbital", "third_person"};
        const char* cameraNames[] = {"FPS", "Orbital", "Third-Person"};

        for (int i = 0; i < 3; i++) {
            bool isActive = (m_CameraManager.GetActiveName() == cameras[i]);
            if (ImGui::RadioButton(cameraNames[i], isActive)) {
                m_CameraManager.SetActive(cameras[i]);
                if (i == 0) {
                    Engine::Input::SetCursorMode(false);
                } else {
                    Engine::Input::SetCursorMode(true);
                }
                if (i == 2) {
                    UpdateThirdPersonTarget();
                }
            }
        }

        ImGui::Separator();

        auto camName = m_CameraManager.GetActiveName();
        if (camName == "fps") {
            ImGui::Text("Controls:");
            ImGui::BulletText("WASD: Move");
            ImGui::BulletText("Mouse: Look around");
            ImGui::BulletText("Shift: Sprint");
            ImGui::BulletText("Tab: Toggle cursor");
        } else if (camName == "orbital") {
            ImGui::Text("Controls:");
            ImGui::BulletText("Left Drag: Orbit");
            ImGui::BulletText("Middle Drag: Pan");
            ImGui::BulletText("Scroll: Zoom");
        } else {
            ImGui::Text("Controls:");
            ImGui::BulletText("Follows animated character");
            ImGui::BulletText("Left Drag: Rotate view");
            ImGui::BulletText("Scroll: Adjust distance");
        }

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);

        ImGui::End();
    }

private:
    void CreateEnvironment() {
        // Ground
        CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {60.0f, 0.5f, 60.0f},
            {0.2f, 0.25f, 0.2f, 1.0f},
            0.0f, 0.8f
        );

        // Central platform
        CreateObject(m_CubeMesh,
            {0.0f, 0.25f, 0.0f},
            {10.0f, 0.5f, 10.0f},
            {0.4f, 0.4f, 0.45f, 1.0f},
            0.1f, 0.6f
        );

        // Pillars around platform
        for (int i = 0; i < 8; i++) {
            float angle = static_cast<float>(i) * glm::quarter_pi<float>();
            float x = glm::cos(angle) * 8.0f;
            float z = glm::sin(angle) * 8.0f;

            CreateObject(m_CubeMesh,
                {x, 2.0f, z},
                {1.0f, 4.0f, 1.0f},
                {0.6f, 0.6f, 0.65f, 1.0f},
                0.3f, 0.5f
            );
        }

        // Outer ring of objects for depth reference
        for (int i = 0; i < 16; i++) {
            float angle = static_cast<float>(i) * glm::pi<float>() / 8.0f;
            float x = glm::cos(angle) * 25.0f;
            float z = glm::sin(angle) * 25.0f;

            CreateObject(m_SphereMesh,
                {x, 1.5f, z},
                {2.0f, 2.0f, 2.0f},
                {0.8f, 0.3f, 0.3f, 1.0f},
                0.0f, 0.4f
            );
        }
    }

    void CreateCharacter() {
        // Simple character representation (for third-person demo)
        // Body
        m_CharacterBody = CreateObject(m_CubeMesh,
            {0.0f, 1.5f, 0.0f},
            {0.8f, 1.5f, 0.5f},
            {0.2f, 0.4f, 0.8f, 1.0f},
            0.0f, 0.5f
        );

        // Head
        m_CharacterHead = CreateObject(m_SphereMesh,
            {0.0f, 2.8f, 0.0f},
            {0.5f, 0.5f, 0.5f},
            {0.9f, 0.75f, 0.6f, 1.0f},
            0.0f, 0.6f
        );
    }

    void AnimateCharacter(Engine::f32 dt) {
        // Character walks in a figure-8 pattern
        float speed = 0.3f;
        float t = m_Time * speed;

        // Figure-8 path
        float x = glm::sin(t) * 6.0f;
        float z = glm::sin(t * 2.0f) * 3.0f;

        // Calculate facing direction
        float nextT = (m_Time + dt) * speed;
        float nextX = glm::sin(nextT) * 6.0f;
        float nextZ = glm::sin(nextT * 2.0f) * 3.0f;
        glm::vec3 dir = glm::normalize(glm::vec3(nextX - x, 0.0f, nextZ - z));
        float facing = glm::atan(dir.x, dir.z);

        // Update body
        auto& bodyT = m_Registry.get<Engine::Transform>(m_CharacterBody);
        bodyT.SetPosition(x, 1.25f + glm::sin(m_Time * 8.0f) * 0.1f, z);  // Subtle bobbing
        bodyT.SetRotation(glm::angleAxis(facing, glm::vec3(0.0f, 1.0f, 0.0f)));
        bodyT.UpdateWorldMatrix();

        // Update head
        auto& headT = m_Registry.get<Engine::Transform>(m_CharacterHead);
        headT.SetPosition(x, 2.5f + glm::sin(m_Time * 8.0f) * 0.1f, z);
        headT.UpdateWorldMatrix();

        m_CharacterPosition = glm::vec3(x, 1.5f, z);
    }

    void UpdateThirdPersonTarget() {
        auto* tpCamera = dynamic_cast<Engine::ThirdPersonCameraController*>(
            m_CameraManager.GetActive()
        );
        if (tpCamera) {
            tpCamera->SetTarget(m_CharacterPosition);
        }
    }

    void CreateLighting() {
        CreateAmbientLight({0.1f, 0.12f, 0.15f}, 1.0f);

        CreateDirectionalLight(
            {0.4f, -0.8f, 0.3f},
            {1.0f, 0.95f, 0.9f},
            0.8f,
            true
        );

        // Accent lights
        CreatePointLight({0.0f, 8.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 15.0f, 20.0f);
        CreatePointLight({-15.0f, 4.0f, 0.0f}, {0.5f, 0.8f, 1.0f}, 10.0f, 15.0f);
        CreatePointLight({15.0f, 4.0f, 0.0f}, {1.0f, 0.8f, 0.5f}, 10.0f, 15.0f);
    }

private:
    entt::entity m_CharacterBody;
    entt::entity m_CharacterHead;
    glm::vec3 m_CharacterPosition = {0.0f, 1.5f, 0.0f};
    bool m_CursorEnabled = true;
};

REGISTER_DEMO(CameraSystemsDemo)

} // namespace Demos
