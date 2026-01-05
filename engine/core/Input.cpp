#include "Input.hpp"
#include <GLFW/glfw3.h>
#include <cstring>
#include <imgui.h>

namespace Engine {

GLFWwindow* Input::s_Window = nullptr;

bool Input::s_CurrentKeys[MAX_KEYS] = {};
bool Input::s_PreviousKeys[MAX_KEYS] = {};

bool Input::s_CurrentMouseButtons[MAX_MOUSE_BUTTONS] = {};
bool Input::s_PreviousMouseButtons[MAX_MOUSE_BUTTONS] = {};

glm::vec2 Input::s_MousePosition{0.0f};
glm::vec2 Input::s_LastMousePosition{0.0f};
glm::vec2 Input::s_MouseDelta{0.0f};

glm::vec2 Input::s_ScrollDelta{0.0f};
glm::vec2 Input::s_ScrollAccumulator{0.0f};

bool Input::s_FirstMouse = true;

void Input::Init(GLFWwindow* window) {
    s_Window = window;

    // Initialize all arrays to false
    std::memset(s_CurrentKeys, 0, sizeof(s_CurrentKeys));
    std::memset(s_PreviousKeys, 0, sizeof(s_PreviousKeys));
    std::memset(s_CurrentMouseButtons, 0, sizeof(s_CurrentMouseButtons));
    std::memset(s_PreviousMouseButtons, 0, sizeof(s_PreviousMouseButtons));

    // Get initial mouse position
    double xpos, ypos;
    glfwGetCursorPos(s_Window, &xpos, &ypos);
    s_MousePosition = { static_cast<f32>(xpos), static_cast<f32>(ypos) };
    s_LastMousePosition = s_MousePosition;
    s_FirstMouse = true;
}

void Input::Update() {
    // Copy current state to previous
    std::memcpy(s_PreviousKeys, s_CurrentKeys, sizeof(s_CurrentKeys));
    std::memcpy(s_PreviousMouseButtons, s_CurrentMouseButtons, sizeof(s_CurrentMouseButtons));

    // Update current key states - only query valid GLFW key ranges
    // GLFW valid keys: 32-96 (printable), 256-348 (special keys)
    auto updateKey = [](i32 key) {
        auto state = glfwGetKey(s_Window, key);
        s_CurrentKeys[key] = (state == GLFW_PRESS || state == GLFW_REPEAT);
    };

    for (i32 i = 32; i <= 96; i++) updateKey(i);
    for (i32 i = 256; i <= 348; i++) updateKey(i);

    // Update current mouse button states
    for (i32 i = 0; i < MAX_MOUSE_BUTTONS; i++) {
        auto state = glfwGetMouseButton(s_Window, i);
        s_CurrentMouseButtons[i] = (state == GLFW_PRESS);
    }

    // Update mouse position and delta
    s_LastMousePosition = s_MousePosition;

    double xpos, ypos;
    glfwGetCursorPos(s_Window, &xpos, &ypos);
    s_MousePosition = { static_cast<f32>(xpos), static_cast<f32>(ypos) };

    if (s_FirstMouse) {
        s_LastMousePosition = s_MousePosition;
        s_FirstMouse = false;
    }

    s_MouseDelta = s_MousePosition - s_LastMousePosition;

    // Transfer scroll accumulator to delta and reset
    s_ScrollDelta = s_ScrollAccumulator;
    s_ScrollAccumulator = glm::vec2(0.0f);
}

bool Input::IsKeyPressed(i32 keycode) {
    if (keycode < 0 || keycode >= MAX_KEYS) return false;
    return s_CurrentKeys[keycode];
}

bool Input::IsKeyJustPressed(i32 keycode) {
    if (keycode < 0 || keycode >= MAX_KEYS) return false;
    return s_CurrentKeys[keycode] && !s_PreviousKeys[keycode];
}

bool Input::IsKeyJustReleased(i32 keycode) {
    if (keycode < 0 || keycode >= MAX_KEYS) return false;
    return !s_CurrentKeys[keycode] && s_PreviousKeys[keycode];
}

bool Input::IsMouseButtonPressed(i32 button) {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
    return s_CurrentMouseButtons[button];
}

bool Input::IsMouseButtonJustPressed(i32 button) {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
    return s_CurrentMouseButtons[button] && !s_PreviousMouseButtons[button];
}

bool Input::IsMouseButtonJustReleased(i32 button) {
    if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
    return !s_CurrentMouseButtons[button] && s_PreviousMouseButtons[button];
}

glm::vec2 Input::GetMousePosition() {
    return s_MousePosition;
}

f32 Input::GetMouseX() {
    return s_MousePosition.x;
}

f32 Input::GetMouseY() {
    return s_MousePosition.y;
}

glm::vec2 Input::GetMouseDelta() {
    return s_MouseDelta;
}

f32 Input::GetMouseDeltaX() {
    return s_MouseDelta.x;
}

f32 Input::GetMouseDeltaY() {
    return s_MouseDelta.y;
}

glm::vec2 Input::GetScrollDelta() {
    return s_ScrollDelta;
}

void Input::SetScrollOffset(f32 xOffset, f32 yOffset) {
    s_ScrollAccumulator.x += xOffset;
    s_ScrollAccumulator.y += yOffset;
}

void Input::SetCursorMode(bool enabled) {
    glfwSetInputMode(s_Window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    if (!enabled) {
        // Reset first mouse flag when disabling cursor to avoid jump
        s_FirstMouse = true;
    }
}

// ImGui awareness
bool Input::IsImGuiCapturingMouse() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    return ctx && ImGui::GetIO().WantCaptureMouse;
}

bool Input::IsImGuiCapturingKeyboard() {
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    return ctx && ImGui::GetIO().WantCaptureKeyboard;
}

// Game-aware input functions
bool Input::IsGameKeyPressed(i32 keycode) {
    if (IsImGuiCapturingKeyboard()) return false;
    return IsKeyPressed(keycode);
}

bool Input::IsGameKeyJustPressed(i32 keycode) {
    if (IsImGuiCapturingKeyboard()) return false;
    return IsKeyJustPressed(keycode);
}

bool Input::IsGameKeyJustReleased(i32 keycode) {
    if (IsImGuiCapturingKeyboard()) return false;
    return IsKeyJustReleased(keycode);
}

bool Input::IsGameMouseButtonPressed(i32 button) {
    if (IsImGuiCapturingMouse()) return false;
    return IsMouseButtonPressed(button);
}

bool Input::IsGameMouseButtonJustPressed(i32 button) {
    if (IsImGuiCapturingMouse()) return false;
    return IsMouseButtonJustPressed(button);
}

bool Input::IsGameMouseButtonJustReleased(i32 button) {
    if (IsImGuiCapturingMouse()) return false;
    return IsMouseButtonJustReleased(button);
}

glm::vec2 Input::GetGameMouseDelta() {
    if (IsImGuiCapturingMouse()) return glm::vec2(0.0f);
    return GetMouseDelta();
}

glm::vec2 Input::GetGameScrollDelta() {
    if (IsImGuiCapturingMouse()) return glm::vec2(0.0f);
    return GetScrollDelta();
}

} // namespace Engine
