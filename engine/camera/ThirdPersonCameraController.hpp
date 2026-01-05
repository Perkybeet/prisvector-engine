#pragma once

#include "CameraController.hpp"
#include "PerspectiveCamera.hpp"
#include "events/MouseEvents.hpp"
#include "events/WindowEvents.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct ThirdPersonCameraSettings {
    f32 Distance = 5.0f;
    f32 MinDistance = 2.0f;
    f32 MaxDistance = 20.0f;
    f32 HeightOffset = 1.5f;
    f32 RotationSpeed = 0.3f;
    f32 ZoomSpeed = 1.0f;
    f32 Smoothing = 10.0f;
    f32 FOV = 45.0f;
    f32 NearPlane = 0.1f;
    f32 FarPlane = 1000.0f;
    f32 InitialYaw = 0.0f;
    f32 InitialPitch = 20.0f;
    f32 MinPitch = -60.0f;
    f32 MaxPitch = 80.0f;
};

class ThirdPersonCameraController : public CameraController {
public:
    explicit ThirdPersonCameraController(f32 aspectRatio);
    ThirdPersonCameraController(f32 aspectRatio, const ThirdPersonCameraSettings& settings);

    void OnUpdate(f32 deltaTime) override;
    void OnEvent(Event& event) override;

    Camera& GetCamera() override { return m_Camera; }
    const Camera& GetCamera() const override { return m_Camera; }

    // Set the target position the camera follows
    void SetTarget(const glm::vec3& target) { m_TargetPosition = target; }
    const glm::vec3& GetTarget() const { return m_TargetPosition; }

    // Camera parameters
    void SetDistance(f32 distance);
    f32 GetDistance() const { return m_Distance; }

    void SetHeightOffset(f32 offset) { m_HeightOffset = offset; }
    void SetRotationSpeed(f32 speed) { m_RotationSpeed = speed; }
    void SetSmoothing(f32 smoothing) { m_Smoothing = smoothing; }

    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    f32 GetYaw() const { return m_Yaw; }
    f32 GetPitch() const { return m_Pitch; }

private:
    void UpdateCameraPosition(f32 deltaTime);

    bool OnMouseMoved(MouseMovedEvent& event);
    bool OnMouseScrolled(MouseScrolledEvent& event);
    bool OnWindowResize(WindowResizeEvent& event);

private:
    PerspectiveCamera m_Camera;

    glm::vec3 m_TargetPosition{0.0f};
    glm::vec3 m_CurrentPosition{0.0f};

    f32 m_Distance = 5.0f;
    f32 m_TargetDistance = 5.0f;
    f32 m_MinDistance = 2.0f;
    f32 m_MaxDistance = 20.0f;

    f32 m_HeightOffset = 1.5f;
    f32 m_RotationSpeed = 0.3f;
    f32 m_ZoomSpeed = 1.0f;
    f32 m_Smoothing = 10.0f;

    f32 m_Yaw = 0.0f;
    f32 m_Pitch = 20.0f;
    f32 m_TargetYaw = 0.0f;
    f32 m_TargetPitch = 20.0f;
    f32 m_MinPitch = -60.0f;
    f32 m_MaxPitch = 80.0f;

    glm::vec2 m_LastMousePosition{0.0f};
    bool m_FirstMouse = true;
    bool m_Enabled = true;
    bool m_Rotating = false;
};

} // namespace Engine
