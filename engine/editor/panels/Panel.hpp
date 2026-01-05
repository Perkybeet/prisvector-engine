#pragma once

#include "core/Types.hpp"
#include "events/Event.hpp"
#include "../EditorContext.hpp"

namespace Editor {

class Panel {
public:
    Panel(const Engine::String& name) : m_Name(name) {}
    virtual ~Panel() = default;

    virtual void OnInit(EditorContext& context) { m_Context = &context; }
    virtual void OnShutdown() {}
    virtual void OnUpdate(Engine::f32 deltaTime) { (void)deltaTime; }
    virtual void OnImGuiRender() = 0;
    virtual void OnEvent(Engine::Event& event) { (void)event; }
    virtual void OnResize(Engine::u32 width, Engine::u32 height) { (void)width; (void)height; }

    void SetVisible(bool visible) { m_Visible = visible; }
    bool IsVisible() const { return m_Visible; }
    const Engine::String& GetName() const { return m_Name; }

protected:
    EditorContext* m_Context = nullptr;
    Engine::String m_Name;
    bool m_Visible = true;
};

} // namespace Editor
