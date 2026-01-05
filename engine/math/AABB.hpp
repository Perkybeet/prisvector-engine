#pragma once

#include "core/Types.hpp"
#include <glm/glm.hpp>
#include <algorithm>

namespace Engine {

struct AABB {
    glm::vec3 Min{0.0f};
    glm::vec3 Max{0.0f};

    AABB() = default;

    AABB(const glm::vec3& min, const glm::vec3& max)
        : Min(min), Max(max) {}

    // Create AABB from center and half-extents
    static AABB FromCenterExtents(const glm::vec3& center, const glm::vec3& extents) {
        return AABB(center - extents, center + extents);
    }

    // Create unit cube AABB (-0.5 to 0.5)
    static AABB UnitCube() {
        return AABB(glm::vec3(-0.5f), glm::vec3(0.5f));
    }

    glm::vec3 GetCenter() const {
        return (Min + Max) * 0.5f;
    }

    glm::vec3 GetExtents() const {
        return (Max - Min) * 0.5f;
    }

    glm::vec3 GetSize() const {
        return Max - Min;
    }

    // Get all 8 corners of the AABB
    void GetCorners(glm::vec3 corners[8]) const {
        corners[0] = glm::vec3(Min.x, Min.y, Min.z);
        corners[1] = glm::vec3(Max.x, Min.y, Min.z);
        corners[2] = glm::vec3(Min.x, Max.y, Min.z);
        corners[3] = glm::vec3(Max.x, Max.y, Min.z);
        corners[4] = glm::vec3(Min.x, Min.y, Max.z);
        corners[5] = glm::vec3(Max.x, Min.y, Max.z);
        corners[6] = glm::vec3(Min.x, Max.y, Max.z);
        corners[7] = glm::vec3(Max.x, Max.y, Max.z);
    }

    // Transform AABB by a matrix (returns new AABB that encloses transformed box)
    AABB Transform(const glm::mat4& matrix) const {
        glm::vec3 corners[8];
        GetCorners(corners);

        glm::vec3 newMin(std::numeric_limits<f32>::max());
        glm::vec3 newMax(std::numeric_limits<f32>::lowest());

        for (int i = 0; i < 8; i++) {
            glm::vec4 transformed = matrix * glm::vec4(corners[i], 1.0f);
            glm::vec3 point = glm::vec3(transformed);

            newMin = glm::min(newMin, point);
            newMax = glm::max(newMax, point);
        }

        return AABB(newMin, newMax);
    }

    // Check if point is inside AABB
    bool Contains(const glm::vec3& point) const {
        return point.x >= Min.x && point.x <= Max.x &&
               point.y >= Min.y && point.y <= Max.y &&
               point.z >= Min.z && point.z <= Max.z;
    }

    // Check if two AABBs intersect
    bool Intersects(const AABB& other) const {
        return Min.x <= other.Max.x && Max.x >= other.Min.x &&
               Min.y <= other.Max.y && Max.y >= other.Min.y &&
               Min.z <= other.Max.z && Max.z >= other.Min.z;
    }

    // Expand AABB to include a point
    void ExpandToInclude(const glm::vec3& point) {
        Min = glm::min(Min, point);
        Max = glm::max(Max, point);
    }

    // Expand AABB to include another AABB
    void ExpandToInclude(const AABB& other) {
        Min = glm::min(Min, other.Min);
        Max = glm::max(Max, other.Max);
    }
};

// Bounding sphere for faster rough culling
struct BoundingSphere {
    glm::vec3 Center{0.0f};
    f32 Radius = 0.0f;

    BoundingSphere() = default;

    BoundingSphere(const glm::vec3& center, f32 radius)
        : Center(center), Radius(radius) {}

    // Create bounding sphere from AABB
    static BoundingSphere FromAABB(const AABB& aabb) {
        glm::vec3 center = aabb.GetCenter();
        f32 radius = glm::length(aabb.GetExtents());
        return BoundingSphere(center, radius);
    }

    // Transform sphere by matrix (uniform scale only for accurate results)
    BoundingSphere Transform(const glm::mat4& matrix) const {
        glm::vec4 transformedCenter = matrix * glm::vec4(Center, 1.0f);

        // Extract scale from matrix (approximate, assumes uniform scale)
        f32 scaleX = glm::length(glm::vec3(matrix[0]));
        f32 scaleY = glm::length(glm::vec3(matrix[1]));
        f32 scaleZ = glm::length(glm::vec3(matrix[2]));
        f32 maxScale = std::max({scaleX, scaleY, scaleZ});

        return BoundingSphere(glm::vec3(transformedCenter), Radius * maxScale);
    }
};

} // namespace Engine
