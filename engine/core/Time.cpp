#include "Time.hpp"
#include <GLFW/glfw3.h>

namespace Engine {

f32 Time::s_DeltaTime = 0.0f;
f32 Time::s_LastFrameTime = 0.0f;
f32 Time::s_TimeScale = 1.0f;

void Time::Init() {
    s_LastFrameTime = static_cast<f32>(glfwGetTime());
}

void Time::Update() {
    f32 currentTime = static_cast<f32>(glfwGetTime());
    s_DeltaTime = currentTime - s_LastFrameTime;
    s_LastFrameTime = currentTime;
}

f32 Time::GetTime() {
    return static_cast<f32>(glfwGetTime());
}

} // namespace Engine
