#include "EditorCamera.hpp"
#include "core/Input.hpp"
#include <imgui.h>
#include <ImGuizmo.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace Editor {

EditorCamera::EditorCamera(Engine::f32 fov, Engine::f32 aspectRatio,
                           Engine::f32 nearClip, Engine::f32 farClip)
    : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip)
{
    m_Camera.SetPerspective(fov, aspectRatio, nearClip, farClip);
    UpdateView();
}

void EditorCamera::OnUpdate(Engine::f32 deltaTime) {
    (void)deltaTime;

    // Don't process camera input when gizmo is being used
    if (ImGuizmo::IsUsing()) return;

    bool altPressed = Engine::Input::IsKeyPressed(Engine::Key::LeftAlt) ||
                      Engine::Input::IsKeyPressed(Engine::Key::RightAlt);

    glm::vec2 mousePos = Engine::Input::GetMousePosition();
    glm::vec2 delta = (mousePos - m_LastMousePosition) * 0.003f;
    m_LastMousePosition = mousePos;

    if (altPressed) {
        if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Middle)) {
            Pan(delta);
        } else if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Left)) {
            Rotate(delta);
        } else if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Right)) {
            Zoom(delta.y);
        }
    }

    UpdateView();
}

void EditorCamera::OnEvent(Engine::Event& event) {
    Engine::EventDispatcher dispatcher(event);
    dispatcher.Dispatch<Engine::MouseScrolledEvent>(
        [this](Engine::MouseScrolledEvent& e) { return OnMouseScroll(e); });
}

bool EditorCamera::OnMouseScroll(Engine::MouseScrolledEvent& e) {
    float delta = e.GetYOffset() * 0.1f;
    Zoom(delta);  // Scroll up = zoom in, scroll down = zoom out
    UpdateView();
    return false;
}

void EditorCamera::Pan(const glm::vec2& delta) {
    auto speed = GetPanSpeed();
    m_FocalPoint -= GetRight() * delta.x * speed.x * m_Distance;
    m_FocalPoint += GetUp() * delta.y * speed.y * m_Distance;
}

void EditorCamera::Rotate(const glm::vec2& delta) {
    float yawSign = GetUp().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * m_RotationSpeed;
    m_Pitch += delta.y * m_RotationSpeed;

    // Clamp pitch
    m_Pitch = std::clamp(m_Pitch, -1.5f, 1.5f);
}

void EditorCamera::Zoom(Engine::f32 delta) {
    m_Distance -= delta * GetZoomSpeedFactor();
    m_Distance = std::max(m_Distance, 0.5f);
}

void EditorCamera::UpdateView() {
    glm::quat orientation = glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    m_Position = m_FocalPoint - GetForward() * m_Distance;
    m_Camera.SetView(m_Position, GetForward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 EditorCamera::GetForward() const {
    glm::quat orientation = glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    return glm::rotate(orientation, glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 EditorCamera::GetRight() const {
    glm::quat orientation = glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    return glm::rotate(orientation, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 EditorCamera::GetUp() const {
    glm::quat orientation = glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    return glm::rotate(orientation, glm::vec3(0.0f, 1.0f, 0.0f));
}

void EditorCamera::FocusOn(const glm::vec3& target, Engine::f32 distance) {
    m_FocalPoint = target;
    if (distance > 0.0f) {
        m_Distance = distance;
    }
    UpdateView();
}

void EditorCamera::FocusOnBounds(const Engine::AABB& bounds) {
    glm::vec3 center = bounds.GetCenter();
    glm::vec3 extents = bounds.GetExtents();
    float maxExtent = std::max({extents.x, extents.y, extents.z});
    float distance = maxExtent * 2.0f / std::tan(glm::radians(m_FOV) * 0.5f);
    FocusOn(center, distance);
}

void EditorCamera::ResetView() {
    m_FocalPoint = glm::vec3(0.0f);
    m_Distance = 10.0f;
    m_Pitch = 0.3f;
    m_Yaw = 0.5f;
    UpdateView();
}

void EditorCamera::SetViewportSize(Engine::f32 width, Engine::f32 height) {
    if (width > 0 && height > 0) {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        m_AspectRatio = width / height;
        m_Camera.SetPerspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
    }
}

glm::vec2 EditorCamera::GetPanSpeed() const {
    float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

Engine::f32 EditorCamera::GetZoomSpeedFactor() const {
    float distance = m_Distance * 0.2f;
    distance = std::max(distance, 0.0f);
    float speed = distance * distance;
    speed = std::clamp(speed, 0.001f, 100.0f);
    return speed * m_ZoomSpeed;
}

} // namespace Editor
