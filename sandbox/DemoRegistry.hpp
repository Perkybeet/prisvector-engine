#pragma once

#include "DemoBase.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace Demos {

// Demo info for registration
struct DemoInfo {
    const char* Name;
    const char* Description;
    std::function<std::unique_ptr<DemoBase>()> CreateFn;
};

// Registry of all available demos
class DemoRegistry {
public:
    static DemoRegistry& Instance() {
        static DemoRegistry instance;
        return instance;
    }

    void Register(const char* name, const char* description,
                  std::function<std::unique_ptr<DemoBase>()> createFn) {
        m_Demos.push_back({name, description, std::move(createFn)});
    }

    const std::vector<DemoInfo>& GetDemos() const { return m_Demos; }

    std::unique_ptr<DemoBase> Create(size_t index) {
        if (index < m_Demos.size()) {
            return m_Demos[index].CreateFn();
        }
        return nullptr;
    }

    std::unique_ptr<DemoBase> Create(const char* name) {
        for (const auto& demo : m_Demos) {
            if (strcmp(demo.Name, name) == 0) {
                return demo.CreateFn();
            }
        }
        return nullptr;
    }

private:
    std::vector<DemoInfo> m_Demos;
};

// Helper macro to register a demo
#define REGISTER_DEMO(DemoClass) \
    namespace { \
        struct DemoClass##Registrar { \
            DemoClass##Registrar() { \
                auto instance = std::make_unique<DemoClass>(); \
                DemoRegistry::Instance().Register( \
                    instance->GetName(), \
                    instance->GetDescription(), \
                    []() { return std::make_unique<DemoClass>(); } \
                ); \
            } \
        }; \
        static DemoClass##Registrar s_##DemoClass##Registrar; \
    }

// Demo Launcher Application
class DemoLauncher : public Engine::Application {
public:
    DemoLauncher() : Application("Game Engine Demo Launcher", 1600, 900) {}

protected:
    void OnInit() override {
        LOG_INFO("Demo Launcher initialized");
        LOG_INFO("Available demos:");

        auto& demos = DemoRegistry::Instance().GetDemos();
        for (size_t i = 0; i < demos.size(); i++) {
            LOG_INFO("  [{}] {} - {}", i + 1, demos[i].Name, demos[i].Description);
        }

        // Start with first demo if available
        if (!demos.empty()) {
            SwitchDemo(0);
        }
    }

    void OnShutdown() override {
        if (m_CurrentDemo) {
            m_CurrentDemo->OnShutdown();
            m_CurrentDemo.reset();
        }
        LOG_INFO("Demo Launcher shutdown");
    }

    void OnUpdate(Engine::f32 dt) override {
        // Handle demo switching with number keys 1-9
        Engine::i32 keys[] = {
            Engine::Key::D1, Engine::Key::D2, Engine::Key::D3,
            Engine::Key::D4, Engine::Key::D5, Engine::Key::D6,
            Engine::Key::D7, Engine::Key::D8, Engine::Key::D9
        };
        for (int i = 0; i < 9; i++) {
            if (Engine::Input::IsKeyJustPressed(keys[i])) {
                size_t demoIndex = static_cast<size_t>(i);
                if (demoIndex < DemoRegistry::Instance().GetDemos().size()) {
                    SwitchDemo(demoIndex);
                }
            }
        }

        if (Engine::Input::IsKeyJustPressed(Engine::Key::Escape)) {
            Close();
        }

        if (m_CurrentDemo) {
            m_CurrentDemo->OnUpdate(dt);
        }
    }

    void OnAppEvent(Engine::Event& e) override {
        if (e.GetEventType() == Engine::EventType::WindowResize) {
            auto& re = static_cast<Engine::WindowResizeEvent&>(e);
            if (m_CurrentDemo) {
                m_CurrentDemo->OnResize(re.GetWidth(), re.GetHeight());
            }
        }

        if (m_CurrentDemo) {
            m_CurrentDemo->OnEvent(e);
        }
    }

    void OnRender() override {
        if (m_CurrentDemo) {
            m_CurrentDemo->OnRender();
        }
    }

    void OnImGuiRender() override {
        RenderDemoSelector();

        if (m_CurrentDemo) {
            m_CurrentDemo->OnImGuiRender();
        }
    }

private:
    void SwitchDemo(size_t index) {
        if (m_CurrentDemo) {
            m_CurrentDemo->OnShutdown();
        }

        m_CurrentDemo = DemoRegistry::Instance().Create(index);

        if (m_CurrentDemo) {
            m_CurrentDemo->SetWindow(&GetWindow());
            m_CurrentDemo->OnInit();
            m_CurrentDemoIndex = index;
            LOG_INFO("Switched to demo: {}", m_CurrentDemo->GetName());
        }
    }

    void RenderDemoSelector() {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);

        ImGui::Begin("Demo Selector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        auto& demos = DemoRegistry::Instance().GetDemos();
        for (size_t i = 0; i < demos.size(); i++) {
            bool isSelected = (i == m_CurrentDemoIndex);
            char label[128];
            snprintf(label, sizeof(label), "[%zu] %s", i + 1, demos[i].Name);

            if (ImGui::Selectable(label, isSelected)) {
                SwitchDemo(i);
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", demos[i].Description);
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Press 1-9 to switch demos");
        ImGui::TextDisabled("ESC to exit");

        ImGui::End();
    }

private:
    std::unique_ptr<DemoBase> m_CurrentDemo;
    size_t m_CurrentDemoIndex = 0;
};

} // namespace Demos
