#pragma once

#include "camera/PerspectiveCamera.hpp"
#include "events/Event.hpp"
#include "events/MouseEvents.hpp"
#include "events/WindowEvents.hpp"
#include "math/AABB.hpp"
#include <glm/glm.hpp>

namespace Editor {

class EditorCamera {
public:
    EditorCamera(Engine::f32 fov = 45.0f, Engine::f32 aspectRatio = 16.0f/9.0f,
                 Engine::f32 nearClip = 0.1f, Engine::f32 farClip = 1000.0f);

    void OnUpdate(Engine::f32 deltaTime);
    void OnEvent(Engine::Event& event);

    // Focus controls
    void FocusOn(const glm::vec3& target, Engine::f32 distance = 0.0f);
    void FocusOnBounds(const Engine::AABB& bounds);
    void ResetView();

    // Camera access
    const Engine::Camera& GetCamera() const { return m_Camera; }
    const glm::mat4& GetViewMatrix() const { return m_Camera.GetViewMatrix(); }
    const glm::mat4& GetProjectionMatrix() const { return m_Camera.GetProjectionMatrix(); }
    glm::vec3 GetPosition() const { return m_Position; }
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

    // Viewport
    void SetViewportSize(Engine::f32 width, Engine::f32 height);

    // Settings
    void SetMoveSpeed(Engine::f32 speed) { m_MoveSpeed = speed; }
    void SetRotationSpeed(Engine::f32 speed) { m_RotationSpeed = speed; }
    void SetZoomSpeed(Engine::f32 speed) { m_ZoomSpeed = speed; }

    Engine::f32 GetMoveSpeed() const { return m_MoveSpeed; }
    Engine::f32 GetDistance() const { return m_Distance; }

private:
    void UpdateView();
    void Pan(const glm::vec2& delta);
    void Rotate(const glm::vec2& delta);
    void Zoom(Engine::f32 delta);

    bool OnMouseScroll(Engine::MouseScrolledEvent& e);
    bool OnMouseMoved(Engine::MouseMovedEvent& e);

    glm::vec2 GetPanSpeed() const;
    Engine::f32 GetZoomSpeedFactor() const;

private:
    Engine::PerspectiveCamera m_Camera;

    glm::vec3 m_FocalPoint{0.0f, 0.0f, 0.0f};
    glm::vec3 m_Position{0.0f, 2.0f, 10.0f};

    Engine::f32 m_Distance = 10.0f;
    Engine::f32 m_Pitch = 0.0f;
    Engine::f32 m_Yaw = 0.0f;

    Engine::f32 m_FOV = 45.0f;
    Engine::f32 m_AspectRatio = 16.0f / 9.0f;
    Engine::f32 m_NearClip = 0.1f;
    Engine::f32 m_FarClip = 1000.0f;

    Engine::f32 m_ViewportWidth = 1280.0f;
    Engine::f32 m_ViewportHeight = 720.0f;

    glm::vec2 m_LastMousePosition{0.0f};
    bool m_Panning = false;
    bool m_Rotating = false;

    Engine::f32 m_MoveSpeed = 5.0f;
    Engine::f32 m_RotationSpeed = 0.8f;
    Engine::f32 m_ZoomSpeed = 1.0f;
};

} // namespace Editor
