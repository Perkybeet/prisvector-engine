#pragma once

#include "ecs/Component.hpp"
#include "core/Types.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

// Transform component - cache-friendly layout (128 bytes, 16-byte aligned)
struct alignas(16) Transform {
    // Core transform data
    glm::vec3 Position{0.0f, 0.0f, 0.0f};
    f32 _pad1 = 0.0f;  // Padding for alignment

    glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};  // w, x, y, z

    glm::vec3 Scale{1.0f, 1.0f, 1.0f};
    f32 _pad2 = 0.0f;  // Padding for alignment

    // Cached world matrix (computed by TransformSystem)
    glm::mat4 WorldMatrix{1.0f};

    // Dirty flag for lazy matrix recalculation
    bool Dirty = true;
    u8 _pad3[3] = {0, 0, 0};  // Padding

    // =========================================================================
    // Convenience Methods
    // =========================================================================

    // Set position and mark dirty
    void SetPosition(const glm::vec3& pos) {
        Position = pos;
        Dirty = true;
    }

    void SetPosition(f32 x, f32 y, f32 z) {
        Position = glm::vec3(x, y, z);
        Dirty = true;
    }

    // Translate by delta
    void Translate(const glm::vec3& delta) {
        Position += delta;
        Dirty = true;
    }

    // Set rotation from quaternion
    void SetRotation(const glm::quat& rot) {
        Rotation = glm::normalize(rot);
        Dirty = true;
    }

    // Set rotation from euler angles (in radians)
    void SetRotationEuler(const glm::vec3& euler) {
        Rotation = glm::quat(euler);
        Dirty = true;
    }

    void SetRotationEuler(f32 pitch, f32 yaw, f32 roll) {
        SetRotationEuler(glm::vec3(pitch, yaw, roll));
    }

    // Get euler angles (in radians)
    glm::vec3 GetEulerAngles() const {
        return glm::eulerAngles(Rotation);
    }

    // Rotate by quaternion
    void Rotate(const glm::quat& rot) {
        Rotation = glm::normalize(rot * Rotation);
        Dirty = true;
    }

    // Rotate around axis
    void RotateAround(const glm::vec3& axis, f32 angle) {
        Rotate(glm::angleAxis(angle, glm::normalize(axis)));
    }

    // Set uniform scale
    void SetScale(f32 uniformScale) {
        Scale = glm::vec3(uniformScale);
        Dirty = true;
    }

    // Set non-uniform scale
    void SetScale(const glm::vec3& scale) {
        Scale = scale;
        Dirty = true;
    }

    void SetScale(f32 x, f32 y, f32 z) {
        Scale = glm::vec3(x, y, z);
        Dirty = true;
    }

    // Direction vectors
    glm::vec3 GetForward() const {
        return glm::normalize(Rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 GetRight() const {
        return glm::normalize(Rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 GetUp() const {
        return glm::normalize(Rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Look at target
    void LookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0)) {
        if (glm::length(target - Position) < 0.0001f) return;

        glm::mat4 lookMatrix = glm::lookAt(Position, target, up);
        Rotation = glm::quat_cast(glm::inverse(lookMatrix));
        Dirty = true;
    }

    // Compute local matrix (without parent transform)
    glm::mat4 GetLocalMatrix() const {
        glm::mat4 matrix = glm::mat4(1.0f);
        matrix = glm::translate(matrix, Position);
        matrix *= glm::mat4_cast(Rotation);
        matrix = glm::scale(matrix, Scale);
        return matrix;
    }

    // Update world matrix (called by TransformSystem)
    void UpdateWorldMatrix(const glm::mat4& parentMatrix = glm::mat4(1.0f)) {
        WorldMatrix = parentMatrix * GetLocalMatrix();
        Dirty = false;
    }

    // Get world position (from cached matrix)
    glm::vec3 GetWorldPosition() const {
        return glm::vec3(WorldMatrix[3]);
    }

    // Transform point from local to world space
    glm::vec3 TransformPoint(const glm::vec3& localPoint) const {
        return glm::vec3(WorldMatrix * glm::vec4(localPoint, 1.0f));
    }

    // Transform direction from local to world space
    glm::vec3 TransformDirection(const glm::vec3& localDir) const {
        return glm::vec3(WorldMatrix * glm::vec4(localDir, 0.0f));
    }

    // Inverse transform point from world to local space
    glm::vec3 InverseTransformPoint(const glm::vec3& worldPoint) const {
        return glm::vec3(glm::inverse(WorldMatrix) * glm::vec4(worldPoint, 1.0f));
    }
};

// Static assert to verify size and alignment
static_assert(sizeof(Transform) == 128, "Transform should be 128 bytes");
static_assert(alignof(Transform) == 16, "Transform should be 16-byte aligned");

} // namespace Engine

// Reflection registration
REFLECT_COMPONENT(Engine::Transform,
    .data<&Engine::Transform::Position>("Position"_hs)
    .data<&Engine::Transform::Rotation>("Rotation"_hs)
    .data<&Engine::Transform::Scale>("Scale"_hs)
    .data<&Engine::Transform::Dirty>("Dirty"_hs)
);
