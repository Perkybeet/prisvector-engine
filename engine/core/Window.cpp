#include "Window.hpp"
#include "Logger.hpp"
#include "events/WindowEvents.hpp"
#include "events/KeyEvents.hpp"
#include "events/MouseEvents.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace Engine {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description) {
    LOG_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

Window::Window(const WindowProps& props) {
    Init(props);
}

Window::~Window() {
    Shutdown();
}

void Window::Init(const WindowProps& props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.VSync = props.VSync;

    LOG_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

    if (!s_GLFWInitialized) {
        int success = glfwInit();
        if (!success) {
            LOG_CORE_CRITICAL("Failed to initialize GLFW!");
            return;
        }
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    // OpenGL 4.5 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_Window = glfwCreateWindow(static_cast<int>(props.Width),
                                 static_cast<int>(props.Height),
                                 props.Title.c_str(),
                                 nullptr, nullptr);

    if (!m_Window) {
        LOG_CORE_CRITICAL("Failed to create GLFW window!");
        return;
    }

    glfwMakeContextCurrent(m_Window);

    // Initialize GLAD
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        LOG_CORE_CRITICAL("Failed to initialize GLAD!");
        return;
    }

    LOG_CORE_INFO("OpenGL Info:");
    LOG_CORE_INFO("  Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    LOG_CORE_INFO("  Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    LOG_CORE_INFO("  Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(props.VSync);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.Width = width;
        data.Height = height;

        WindowResizeEvent event(width, height);
        if (data.EventCallback) {
            data.EventCallback(event);
        }
    });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        WindowCloseEvent event;
        if (data.EventCallback) {
            data.EventCallback(event);
        }
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event(key, false);
                if (data.EventCallback) data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event(key);
                if (data.EventCallback) data.EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event(key, true);
                if (data.EventCallback) data.EventCallback(event);
                break;
            }
        }
    });

    glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        KeyTypedEvent event(static_cast<i32>(keycode));
        if (data.EventCallback) data.EventCallback(event);
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int /*mods*/) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event(button);
                if (data.EventCallback) data.EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event(button);
                if (data.EventCallback) data.EventCallback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        MouseScrolledEvent event(static_cast<f32>(xOffset), static_cast<f32>(yOffset));
        if (data.EventCallback) data.EventCallback(event);
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        MouseMovedEvent event(static_cast<f32>(xPos), static_cast<f32>(yPos));
        if (data.EventCallback) data.EventCallback(event);
    });
}

void Window::Shutdown() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void Window::OnUpdate() {
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}

void Window::SetVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
    m_Data.VSync = enabled;
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::SetCursorEnabled(bool enabled) {
    m_CursorEnabled = enabled;
    glfwSetInputMode(m_Window, GLFW_CURSOR,
                     enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

Scope<Window> Window::Create(const WindowProps& props) {
    return CreateScope<Window>(props);
}

} // namespace Engine
