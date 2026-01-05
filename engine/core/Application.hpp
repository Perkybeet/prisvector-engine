#pragma once

#include "Types.hpp"
#include "Window.hpp"
#include "events/Event.hpp"
#include "events/WindowEvents.hpp"
#include "ecs/Registry.hpp"
#include "ecs/SystemScheduler.hpp"

namespace Engine {

class Application {
public:
    Application(const String& name = "Game Engine", u32 width = 1280, u32 height = 720);
    virtual ~Application();

    void Run();
    void Close();

    void OnEvent(Event& e);

    Window& GetWindow() { return *m_Window; }

    // ECS access
    Registry& GetRegistry() { return m_Registry; }
    const Registry& GetRegistry() const { return m_Registry; }
    SystemScheduler& GetSystemScheduler() { return m_SystemScheduler; }
    const SystemScheduler& GetSystemScheduler() const { return m_SystemScheduler; }

    static Application& Get() { return *s_Instance; }

protected:
    virtual void OnInit() {}
    virtual void OnShutdown() {}
    virtual void OnUpdate(f32 deltaTime) { (void)deltaTime; }
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
    virtual void OnPostImGuiRender() {}
    virtual void OnAppEvent(Event& e) { (void)e; }

    // ECS members accessible to derived classes
    Registry m_Registry;
    SystemScheduler m_SystemScheduler;

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);

private:
    Scope<Window> m_Window;
    bool m_Running = true;
    bool m_Minimized = false;

    static Application* s_Instance;
};

} // namespace Engine
