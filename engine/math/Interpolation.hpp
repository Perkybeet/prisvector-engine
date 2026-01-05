#pragma once

#include "core/Types.hpp"
#include <glm/glm.hpp>
#include <cmath>

namespace Engine {

namespace Math {

// Linear interpolation
template<typename T>
inline T Lerp(const T& a, const T& b, f32 t) {
    return a + (b - a) * t;
}

// Smooth interpolation (deceleration curve)
// Good for camera smoothing - fast start, slow end
template<typename T>
inline T SmoothDamp(const T& current, const T& target, f32 smoothTime, f32 deltaTime) {
    f32 omega = 2.0f / smoothTime;
    f32 x = omega * deltaTime;
    f32 exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    return Lerp(target, current, exp);
}

// Exponential decay interpolation
// smoothFactor: higher = faster interpolation (typically 5-20)
template<typename T>
inline T ExpDecay(const T& current, const T& target, f32 smoothFactor, f32 deltaTime) {
    return Lerp(current, target, 1.0f - std::exp(-smoothFactor * deltaTime));
}

// Easing functions (t should be 0-1)
namespace Easing {

inline f32 Linear(f32 t) {
    return t;
}

inline f32 EaseInQuad(f32 t) {
    return t * t;
}

inline f32 EaseOutQuad(f32 t) {
    return t * (2.0f - t);
}

inline f32 EaseInOutQuad(f32 t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline f32 EaseInCubic(f32 t) {
    return t * t * t;
}

inline f32 EaseOutCubic(f32 t) {
    f32 t1 = t - 1.0f;
    return t1 * t1 * t1 + 1.0f;
}

inline f32 EaseInOutCubic(f32 t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

inline f32 EaseOutExpo(f32 t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

inline f32 EaseInOutExpo(f32 t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    if (t < 0.5f) return std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f;
    return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

} // namespace Easing

// Clamp angle to -180 to 180 range
inline f32 NormalizeAngle(f32 angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

// Shortest path angle interpolation
inline f32 LerpAngle(f32 a, f32 b, f32 t) {
    f32 diff = NormalizeAngle(b - a);
    return a + diff * t;
}

} // namespace Math

} // namespace Engine
