#pragma once

#include "CameraController.hpp"
#include "PerspectiveCamera.hpp"
#include "events/MouseEvents.hpp"
#include "events/WindowEvents.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct FPSCameraSettings {
    glm::vec3 InitialPosition{0.0f, 0.0f, 3.0f};
    f32 InitialYaw = -90.0f;
    f32 InitialPitch = 0.0f;
    f32 MovementSpeed = 5.0f;
    f32 SprintMultiplier = 2.0f;
    f32 MouseSensitivity = 0.1f;
    f32 FOV = 45.0f;
    f32 NearPlane = 0.1f;
    f32 FarPlane = 1000.0f;

    // Smoothing settings
    bool EnableSmoothing = true;
    f32 RotationSmoothing = 15.0f;   // Higher = faster response (10-25 typical)
    f32 PositionSmoothing = 12.0f;   // Higher = faster response (8-20 typical)
};

class FPSCameraController : public CameraController {
public:
    explicit FPSCameraController(f32 aspectRatio);
    FPSCameraController(f32 aspectRatio, const FPSCameraSettings& settings);

    void OnUpdate(f32 deltaTime) override;
    void OnEvent(Event& event) override;

    Camera& GetCamera() override { return m_Camera; }
    const Camera& GetCamera() const override { return m_Camera; }
    PerspectiveCamera& GetPerspectiveCamera() { return m_Camera; }

    void SetMovementSpeed(f32 speed) { m_MovementSpeed = speed; }
    void SetMouseSensitivity(f32 sensitivity) { m_MouseSensitivity = sensitivity; }
    void SetPosition(const glm::vec3& position);
    void LookAt(const glm::vec3& target);

    // Smoothing controls
    void SetSmoothingEnabled(bool enabled) { m_SmoothingEnabled = enabled; }
    bool IsSmoothingEnabled() const { return m_SmoothingEnabled; }
    void SetRotationSmoothing(f32 value) { m_RotationSmoothing = value; }
    void SetPositionSmoothing(f32 value) { m_PositionSmoothing = value; }

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    glm::vec3 GetFrontVector() const { return m_Front; }
    glm::vec3 GetRightVector() const { return m_Right; }
    glm::vec3 GetUpVector() const { return m_Up; }

    f32 GetYaw() const { return m_Yaw; }
    f32 GetPitch() const { return m_Pitch; }

private:
    void ProcessKeyboardInput(f32 deltaTime);
    void ProcessMouseMovement(f32 xOffset, f32 yOffset);
    void UpdateCameraVectors();
    void UpdateViewMatrix();

    bool OnMouseMoved(MouseMovedEvent& event);
    bool OnMouseScrolled(MouseScrolledEvent& event);
    bool OnWindowResize(WindowResizeEvent& event);

private:
    PerspectiveCamera m_Camera;

    glm::vec3 m_Position;
    glm::vec3 m_Front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_WorldUp{0.0f, 1.0f, 0.0f};

    f32 m_Yaw = -90.0f;
    f32 m_Pitch = 0.0f;

    static constexpr f32 s_MaxPitch = 89.0f;
    static constexpr f32 s_MinPitch = -89.0f;

    f32 m_MovementSpeed = 5.0f;
    f32 m_SprintMultiplier = 2.0f;
    f32 m_MouseSensitivity = 0.1f;

    // Target values for smooth interpolation
    glm::vec3 m_TargetPosition{0.0f, 0.0f, 3.0f};
    f32 m_TargetYaw = -90.0f;
    f32 m_TargetPitch = 0.0f;

    // Smoothing parameters
    bool m_SmoothingEnabled = true;
    f32 m_RotationSmoothing = 15.0f;
    f32 m_PositionSmoothing = 12.0f;

    glm::vec2 m_LastMousePosition{0.0f, 0.0f};
    bool m_FirstMouse = true;
    bool m_Enabled = true;
};

} // namespace Engine
