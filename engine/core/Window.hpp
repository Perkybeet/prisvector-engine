#pragma once

#include "Types.hpp"
#include "events/Event.hpp"

struct GLFWwindow;

namespace Engine {

struct WindowProps {
    String Title;
    u32 Width;
    u32 Height;
    bool VSync;

    WindowProps(const String& title = "Game Engine",
                u32 width = 1280,
                u32 height = 720,
                bool vsync = true)
        : Title(title), Width(width), Height(height), VSync(vsync) {}
};

class Window {
public:
    Window(const WindowProps& props = WindowProps());
    ~Window();

    void OnUpdate();

    u32 GetWidth() const { return m_Data.Width; }
    u32 GetHeight() const { return m_Data.Height; }
    f32 GetAspectRatio() const { return static_cast<f32>(m_Data.Width) / static_cast<f32>(m_Data.Height); }

    void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
    void SetVSync(bool enabled);
    bool IsVSync() const { return m_Data.VSync; }

    GLFWwindow* GetNativeWindow() const { return m_Window; }

    bool ShouldClose() const;

    // Cursor control
    void SetCursorEnabled(bool enabled);
    bool IsCursorEnabled() const { return m_CursorEnabled; }

    static Scope<Window> Create(const WindowProps& props = WindowProps());

private:
    void Init(const WindowProps& props);
    void Shutdown();

private:
    GLFWwindow* m_Window = nullptr;

    struct WindowData {
        String Title;
        u32 Width = 0;
        u32 Height = 0;
        bool VSync = false;
        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
    bool m_CursorEnabled = true;
};

} // namespace Engine
