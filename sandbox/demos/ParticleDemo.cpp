#include "../DemoBase.hpp"
#include "../DemoRegistry.hpp"
#include "renderer/particles/ParticleSystem.hpp"

namespace Demos {

class ParticleDemo : public DemoBase {
public:
    const char* GetName() const override { return "Particle Effects"; }
    const char* GetDescription() const override {
        return "GPU particle system showcase with fire, smoke, sparks, and more";
    }

    void OnInit() override {
        LOG_INFO("Initializing Particle Effects Demo");

        float aspect = GetWindow().GetAspectRatio();

        Engine::OrbitalCameraSettings orbSettings;
        orbSettings.Radius = 20.0f;
        orbSettings.Elevation = 20.0f;
        orbSettings.Smoothing = 8.0f;
        auto orbCamera = Engine::CreateScope<Engine::OrbitalCameraController>(aspect, orbSettings);

        m_CameraManager.Register("orbital", std::move(orbCamera));
        m_CameraManager.SetActive("orbital");
        Engine::Input::SetCursorMode(true);

        InitializeRenderingSystems();

        // Initialize particle system
        m_ParticleSystem = Engine::CreateScope<Engine::ParticleSystem>();
        m_ParticleSystem->Initialize();

        CreateEnvironment();
        CreateParticleEffects();
        CreateLighting();

        m_Exposure = 1.2f;

        glEnable(GL_DEPTH_TEST);

        LOG_INFO("Particle Demo ready - Press 1-7 to toggle effects");
    }

    void OnShutdown() override {
        m_ParticleSystem->Shutdown();
        ShutdownRenderingSystems();
        Engine::Input::SetCursorMode(true);
    }

    void OnUpdate(Engine::f32 dt) override {
        // Toggle effects with number keys
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D1)) {
            ToggleEmitter(0, "Fire");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D2)) {
            ToggleEmitter(1, "Smoke");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D3)) {
            ToggleEmitter(2, "Sparks");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D4)) {
            ToggleEmitter(3, "Rain");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D5)) {
            ToggleEmitter(4, "Snow");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D6)) {
            ToggleEmitter(5, "Magic");
        }
        if (Engine::Input::IsKeyJustPressed(Engine::Key::D7)) {
            TriggerExplosion();
        }

        // Exposure control
        if (Engine::Input::IsKeyPressed(Engine::Key::Equal)) {
            m_Exposure = glm::min(3.0f, m_Exposure + dt * 1.5f);
        }
        if (Engine::Input::IsKeyPressed(Engine::Key::Minus)) {
            m_Exposure = glm::max(0.1f, m_Exposure - dt * 1.5f);
        }

        m_CameraManager.OnUpdate(dt);
        m_Time += dt;

        // Update particle system
        m_ParticleSystem->SetCamera(const_cast<Engine::Camera*>(m_CameraManager.GetActiveCamera()));
        m_ParticleSystem->Update(dt);
    }

    void OnEvent(Engine::Event& e) override {
        m_CameraManager.OnEvent(e);
    }

    void OnResize(Engine::u32 width, Engine::u32 height) override {
        m_LightingSystem->Resize(width, height);
    }

    void OnRender() override {
        RenderScene();

        // Render particles after scene but before tonemapping
        m_ParticleSystem->Render();

        RenderTonemapped();
    }

    void OnImGuiRender() override {
        ImGui::SetNextWindowPos(ImVec2(10, 120), ImGuiCond_FirstUseEver);

        ImGui::Begin("Particle Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FPS: %.0f", Engine::Time::GetFPS());

        auto& stats = m_ParticleSystem->GetStats();
        ImGui::Text("Emitters: %u active / %u total", stats.ActiveEmitters, stats.TotalEmitters);
        ImGui::Text("Particles: %u alive / %u max", stats.AliveParticles, stats.TotalParticles);

        ImGui::Separator();
        ImGui::Text("Effects (press to toggle):");

        const char* effectNames[] = {"Fire", "Smoke", "Sparks", "Rain", "Snow", "Magic"};
        for (int i = 0; i < 6; i++) {
            bool active = (i < static_cast<int>(m_Emitters.size())) && m_Emitters[i] && m_Emitters[i]->IsPlaying();
            if (ImGui::Checkbox(effectNames[i], &active)) {
                if (i < static_cast<int>(m_Emitters.size()) && m_Emitters[i]) {
                    if (active) m_Emitters[i]->Play();
                    else m_Emitters[i]->Pause();
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("[%d]", i + 1);
        }

        ImGui::Separator();

        if (ImGui::Button("Explosion [7]")) {
            TriggerExplosion();
        }

        ImGui::Separator();

        ImGui::SliderFloat("Exposure", &m_Exposure, 0.1f, 3.0f);

        float timeScale = m_ParticleSystem->GetTimeScale();
        if (ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 2.0f)) {
            m_ParticleSystem->SetTimeScale(timeScale);
        }

        ImGui::End();
    }

private:
    void CreateEnvironment() {
        // Dark ground
        CreateObject(m_CubeMesh,
            {0.0f, -0.25f, 0.0f},
            {40.0f, 0.5f, 40.0f},
            {0.1f, 0.1f, 0.12f, 1.0f},
            0.0f, 0.8f
        );

        // Central pedestal for fire
        CreateObject(m_CubeMesh,
            {0.0f, 0.5f, 0.0f},
            {2.0f, 1.0f, 2.0f},
            {0.3f, 0.25f, 0.2f, 1.0f},
            0.0f, 0.7f
        );

        // Pillars for sparks
        CreateObject(m_CubeMesh,
            {-6.0f, 1.5f, 0.0f},
            {1.0f, 3.0f, 1.0f},
            {0.5f, 0.5f, 0.55f, 1.0f},
            0.8f, 0.3f
        );

        CreateObject(m_CubeMesh,
            {6.0f, 1.5f, 0.0f},
            {1.0f, 3.0f, 1.0f},
            {0.5f, 0.5f, 0.55f, 1.0f},
            0.8f, 0.3f
        );
    }

    void CreateParticleEffects() {
        // Reserve space for emitters
        m_Emitters.resize(7, nullptr);

        // Fire - on the pedestal
        auto fireSettings = Engine::ParticlePresets::Fire();
        fireSettings.Position = glm::vec3(0.0f, 1.0f, 0.0f);
        m_Emitters[0] = m_ParticleSystem->CreateEmitter(fireSettings);

        // Smoke - above fire
        auto smokeSettings = Engine::ParticlePresets::Smoke();
        smokeSettings.Position = glm::vec3(0.0f, 2.0f, 0.0f);
        smokeSettings.SpawnRate = 10.0f;
        m_Emitters[1] = m_ParticleSystem->CreateEmitter(smokeSettings);
        m_Emitters[1]->Pause();  // Start paused

        // Sparks - from left pillar
        auto sparkSettings = Engine::ParticlePresets::Sparks();
        sparkSettings.Position = glm::vec3(-6.0f, 3.0f, 0.0f);
        m_Emitters[2] = m_ParticleSystem->CreateEmitter(sparkSettings);
        m_Emitters[2]->Pause();

        // Rain - overhead
        auto rainSettings = Engine::ParticlePresets::Rain();
        rainSettings.Position = glm::vec3(0.0f, 12.0f, 0.0f);
        rainSettings.ShapeSize = glm::vec3(15.0f, 0.1f, 15.0f);
        m_Emitters[3] = m_ParticleSystem->CreateEmitter(rainSettings);
        m_Emitters[3]->Pause();

        // Snow
        auto snowSettings = Engine::ParticlePresets::Snow();
        snowSettings.Position = glm::vec3(0.0f, 12.0f, 0.0f);
        snowSettings.ShapeSize = glm::vec3(15.0f, 0.1f, 15.0f);
        m_Emitters[4] = m_ParticleSystem->CreateEmitter(snowSettings);
        m_Emitters[4]->Pause();

        // Magic - orbiting
        auto magicSettings = Engine::ParticlePresets::Magic();
        magicSettings.Position = glm::vec3(6.0f, 2.0f, 0.0f);
        m_Emitters[5] = m_ParticleSystem->CreateEmitter(magicSettings);
        m_Emitters[5]->Pause();

        LOG_INFO("Created {} particle emitters", m_Emitters.size());
    }

    void CreateLighting() {
        // Dark ambient
        CreateAmbientLight({0.03f, 0.03f, 0.05f}, 1.0f);

        // Dim overhead light
        CreateDirectionalLight(
            {0.2f, -1.0f, 0.1f},
            {0.4f, 0.4f, 0.5f},
            0.3f,
            true
        );

        // Fire point light (animated)
        m_FireLight = CreatePointLight(
            {0.0f, 1.5f, 0.0f},
            {1.0f, 0.6f, 0.2f},
            15.0f, 10.0f
        );
    }

    void ToggleEmitter(int index, const char* name) {
        if (index >= static_cast<int>(m_Emitters.size()) || !m_Emitters[index]) return;

        if (m_Emitters[index]->IsPlaying()) {
            m_Emitters[index]->Pause();
            LOG_INFO("{} OFF", name);
        } else {
            m_Emitters[index]->Play();
            LOG_INFO("{} ON", name);
        }
    }

    void TriggerExplosion() {
        auto explosionSettings = Engine::ParticlePresets::Explosion();
        explosionSettings.Position = glm::vec3(
            (m_RNG() % 10 - 5) * 1.0f,
            2.0f,
            (m_RNG() % 10 - 5) * 1.0f
        );

        auto* emitter = m_ParticleSystem->CreateEmitter(explosionSettings);
        emitter->Emit(explosionSettings.BurstCount);

        LOG_INFO("BOOM! Explosion at ({:.1f}, {:.1f}, {:.1f})",
                 explosionSettings.Position.x,
                 explosionSettings.Position.y,
                 explosionSettings.Position.z);
    }

private:
    Engine::Scope<Engine::ParticleSystem> m_ParticleSystem;
    Engine::Vector<Engine::ParticleEmitter*> m_Emitters;
    entt::entity m_FireLight;
    std::mt19937 m_RNG{std::random_device{}()};
};

REGISTER_DEMO(ParticleDemo)

} // namespace Demos
