#include "FPSCameraController.hpp"
#include "core/Input.hpp"
#include "math/Interpolation.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Engine {

FPSCameraController::FPSCameraController(f32 aspectRatio)
    : m_Position(0.0f, 0.0f, 3.0f) {
    PerspectiveCameraSettings camSettings;
    camSettings.AspectRatio = aspectRatio;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraVectors();
    UpdateViewMatrix();
}

FPSCameraController::FPSCameraController(f32 aspectRatio, const FPSCameraSettings& settings)
    : m_Position(settings.InitialPosition)
    , m_Yaw(settings.InitialYaw)
    , m_Pitch(settings.InitialPitch)
    , m_MovementSpeed(settings.MovementSpeed)
    , m_SprintMultiplier(settings.SprintMultiplier)
    , m_MouseSensitivity(settings.MouseSensitivity)
    , m_TargetPosition(settings.InitialPosition)
    , m_TargetYaw(settings.InitialYaw)
    , m_TargetPitch(settings.InitialPitch)
    , m_SmoothingEnabled(settings.EnableSmoothing)
    , m_RotationSmoothing(settings.RotationSmoothing)
    , m_PositionSmoothing(settings.PositionSmoothing) {
    PerspectiveCameraSettings camSettings;
    camSettings.FOV = settings.FOV;
    camSettings.AspectRatio = aspectRatio;
    camSettings.NearPlane = settings.NearPlane;
    camSettings.FarPlane = settings.FarPlane;
    m_Camera = PerspectiveCamera(camSettings);

    UpdateCameraVectors();
    UpdateViewMatrix();
}

void FPSCameraController::OnUpdate(f32 deltaTime) {
    if (!m_Enabled) return;

    ProcessKeyboardInput(deltaTime);

    // Apply smoothing interpolation
    if (m_SmoothingEnabled) {
        // Smooth position
        m_Position = Math::ExpDecay(m_Position, m_TargetPosition, m_PositionSmoothing, deltaTime);

        // Smooth rotation
        m_Yaw = Math::ExpDecay(m_Yaw, m_TargetYaw, m_RotationSmoothing, deltaTime);
        m_Pitch = Math::ExpDecay(m_Pitch, m_TargetPitch, m_RotationSmoothing, deltaTime);

        UpdateCameraVectors();
    }

    UpdateViewMatrix();
}

void FPSCameraController::OnEvent(Event& event) {
    if (!m_Enabled) return;

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) { return OnMouseMoved(e); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScrolled(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
}

void FPSCameraController::ProcessKeyboardInput(f32 deltaTime) {
    f32 velocity = m_MovementSpeed * deltaTime;

    if (Input::IsGameKeyPressed(Key::LeftShift)) {
        velocity *= m_SprintMultiplier;
    }

    // Move target position (will be smoothly interpolated to actual position)
    glm::vec3& pos = m_SmoothingEnabled ? m_TargetPosition : m_Position;

    if (Input::IsGameKeyPressed(Key::W)) {
        pos += m_Front * velocity;
    }
    if (Input::IsGameKeyPressed(Key::S)) {
        pos -= m_Front * velocity;
    }
    if (Input::IsGameKeyPressed(Key::A)) {
        pos -= m_Right * velocity;
    }
    if (Input::IsGameKeyPressed(Key::D)) {
        pos += m_Right * velocity;
    }
    if (Input::IsGameKeyPressed(Key::Space)) {
        pos += m_WorldUp * velocity;
    }
    if (Input::IsGameKeyPressed(Key::LeftControl)) {
        pos -= m_WorldUp * velocity;
    }
}

void FPSCameraController::ProcessMouseMovement(f32 xOffset, f32 yOffset) {
    xOffset *= m_MouseSensitivity;
    yOffset *= m_MouseSensitivity;

    if (m_SmoothingEnabled) {
        // Update targets (will be smoothly interpolated in OnUpdate)
        m_TargetYaw += xOffset;
        m_TargetPitch += yOffset;
        m_TargetPitch = std::clamp(m_TargetPitch, s_MinPitch, s_MaxPitch);
    } else {
        // Direct update without smoothing
        m_Yaw += xOffset;
        m_Pitch += yOffset;
        m_Pitch = std::clamp(m_Pitch, s_MinPitch, s_MaxPitch);
        UpdateCameraVectors();
    }
}

void FPSCameraController::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void FPSCameraController::UpdateViewMatrix() {
    m_Camera.SetView(m_Position, m_Front, m_Up);
}

bool FPSCameraController::OnMouseMoved(MouseMovedEvent& event) {
    // Don't process mouse movement when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    f32 xPos = event.GetX();
    f32 yPos = event.GetY();

    if (m_FirstMouse) {
        m_LastMousePosition = {xPos, yPos};
        m_FirstMouse = false;
        return false;
    }

    f32 xOffset = xPos - m_LastMousePosition.x;
    f32 yOffset = m_LastMousePosition.y - yPos;

    m_LastMousePosition = {xPos, yPos};

    ProcessMouseMovement(xOffset, yOffset);
    return false;
}

bool FPSCameraController::OnMouseScrolled(MouseScrolledEvent& event) {
    // Don't process scroll when ImGui wants the mouse
    if (Input::IsImGuiCapturingMouse()) return false;

    f32 fov = m_Camera.GetFOV() - event.GetYOffset() * 2.0f;
    fov = std::clamp(fov, 1.0f, 90.0f);
    m_Camera.SetFOV(fov);
    return false;
}

bool FPSCameraController::OnWindowResize(WindowResizeEvent& event) {
    if (event.GetWidth() > 0 && event.GetHeight() > 0) {
        f32 aspectRatio = static_cast<f32>(event.GetWidth()) / static_cast<f32>(event.GetHeight());
        m_Camera.SetAspectRatio(aspectRatio);
    }
    return false;
}

void FPSCameraController::SetPosition(const glm::vec3& position) {
    m_Position = position;
    m_TargetPosition = position;
    UpdateViewMatrix();
}

void FPSCameraController::LookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - m_Position);

    f32 pitch = glm::degrees(asin(direction.y));
    f32 yaw = glm::degrees(atan2(direction.z, direction.x));

    pitch = std::clamp(pitch, s_MinPitch, s_MaxPitch);

    m_Pitch = pitch;
    m_Yaw = yaw;
    m_TargetPitch = pitch;
    m_TargetYaw = yaw;

    UpdateCameraVectors();
    UpdateViewMatrix();
}

} // namespace Engine
