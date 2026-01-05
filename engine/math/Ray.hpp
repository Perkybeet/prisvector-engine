#pragma once

#include "core/Types.hpp"
#include "AABB.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct Ray {
    glm::vec3 Origin{0.0f};
    glm::vec3 Direction{0.0f, 0.0f, -1.0f};

    Ray() = default;

    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : Origin(origin), Direction(glm::normalize(direction)) {}

    // Get point along ray at distance t
    glm::vec3 GetPoint(f32 t) const {
        return Origin + Direction * t;
    }

    // Ray-AABB intersection test using slab method
    // Returns true if intersects, sets tMin to closest hit distance
    bool IntersectsAABB(const AABB& aabb, f32& tMin) const;

    // Create ray from screen coordinates and camera matrices
    // screenPos: mouse position in screen/window coordinates
    // viewportPos: top-left corner of the viewport in screen coordinates
    // viewportSize: size of the viewport
    // viewMatrix: camera view matrix
    // projectionMatrix: camera projection matrix
    static Ray FromScreenPoint(
        const glm::vec2& screenPos,
        const glm::vec2& viewportPos,
        const glm::vec2& viewportSize,
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix
    );
};

} // namespace Engine
