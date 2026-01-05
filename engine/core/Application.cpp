#include "Application.hpp"
#include "Logger.hpp"
#include "Input.hpp"
#include "Time.hpp"
#include "ecs/System.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>

namespace Engine {

Application* Application::s_Instance = nullptr;

Application::Application(const String& name, u32 width, u32 height) {
    s_Instance = this;

    Logger::Init();
    LOG_CORE_INFO("Initializing Engine...");

    WindowProps props;
    props.Title = name;
    props.Width = width;
    props.Height = height;

    m_Window = Window::Create(props);
    m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });

    Input::Init(m_Window->GetNativeWindow());
    Time::Init();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    (void)io; // Unused warning suppression

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;

    ImGui_ImplGlfw_InitForOpenGL(m_Window->GetNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    LOG_CORE_INFO("Engine initialized successfully!");
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    LOG_CORE_INFO("Shutting down Engine...");
}

void Application::Run() {
    OnInit();

    // Initialize ECS systems
    m_SystemScheduler.Initialize(m_Registry.Raw());
    LOG_CORE_INFO("ECS Systems initialized");

    while (m_Running) {
        Time::Update();
        Input::Update();
        f32 deltaTime = Time::GetDeltaTime();

        if (!m_Minimized) {
            // Update ECS systems (PreUpdate, Update, PostUpdate phases)
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PreUpdate, deltaTime);
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::Update, deltaTime);
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PostUpdate, deltaTime);

            // User update callback
            OnUpdate(deltaTime);

            // Physics phases
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PrePhysics, deltaTime);
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::Physics, deltaTime);
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PostPhysics, deltaTime);

            // Render phases
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PreRender, deltaTime);

            OnRender();

            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::Render, deltaTime);
            m_SystemScheduler.UpdatePhase(m_Registry.Raw(), SystemPhase::PostRender, deltaTime);

            // ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            OnImGuiRender();
            OnPostImGuiRender();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        m_Window->OnUpdate();
    }

    // Shutdown ECS systems
    m_SystemScheduler.Shutdown(m_Registry.Raw());

    OnShutdown();
}

void Application::Close() {
    m_Running = false;
}

void Application::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) {
        return OnWindowClose(event);
    });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
        return OnWindowResize(event);
    });

    OnAppEvent(e);
}

bool Application::OnWindowClose(WindowCloseEvent& /*e*/) {
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e) {
    if (e.GetWidth() == 0 || e.GetHeight() == 0) {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    glViewport(0, 0, e.GetWidth(), e.GetHeight());
    return false;
}

} // namespace Engine
