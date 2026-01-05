#pragma once

#include "core/Types.hpp"
#include "math/AABB.hpp"
#include <glm/glm.hpp>
#include <array>

namespace Engine {

struct Plane {
    glm::vec3 Normal{0.0f, 1.0f, 0.0f};
    f32 Distance = 0.0f;

    Plane() = default;

    Plane(const glm::vec3& normal, f32 distance)
        : Normal(normal), Distance(distance) {}

    // Create plane from normal and point on plane
    static Plane FromPointNormal(const glm::vec3& point, const glm::vec3& normal) {
        glm::vec3 n = glm::normalize(normal);
        return Plane(n, -glm::dot(n, point));
    }

    // Signed distance from point to plane (positive = front, negative = back)
    f32 DistanceToPoint(const glm::vec3& point) const {
        return glm::dot(Normal, point) + Distance;
    }

    void Normalize() {
        f32 length = glm::length(Normal);
        if (length > 0.0f) {
            Normal /= length;
            Distance /= length;
        }
    }
};

class Frustum {
public:
    enum PlaneIndex {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    Frustum() = default;

    // Extract frustum planes from view-projection matrix
    void ExtractPlanes(const glm::mat4& viewProjection) {
        // Left plane
        m_Planes[Left].Normal.x = viewProjection[0][3] + viewProjection[0][0];
        m_Planes[Left].Normal.y = viewProjection[1][3] + viewProjection[1][0];
        m_Planes[Left].Normal.z = viewProjection[2][3] + viewProjection[2][0];
        m_Planes[Left].Distance = viewProjection[3][3] + viewProjection[3][0];

        // Right plane
        m_Planes[Right].Normal.x = viewProjection[0][3] - viewProjection[0][0];
        m_Planes[Right].Normal.y = viewProjection[1][3] - viewProjection[1][0];
        m_Planes[Right].Normal.z = viewProjection[2][3] - viewProjection[2][0];
        m_Planes[Right].Distance = viewProjection[3][3] - viewProjection[3][0];

        // Bottom plane
        m_Planes[Bottom].Normal.x = viewProjection[0][3] + viewProjection[0][1];
        m_Planes[Bottom].Normal.y = viewProjection[1][3] + viewProjection[1][1];
        m_Planes[Bottom].Normal.z = viewProjection[2][3] + viewProjection[2][1];
        m_Planes[Bottom].Distance = viewProjection[3][3] + viewProjection[3][1];

        // Top plane
        m_Planes[Top].Normal.x = viewProjection[0][3] - viewProjection[0][1];
        m_Planes[Top].Normal.y = viewProjection[1][3] - viewProjection[1][1];
        m_Planes[Top].Normal.z = viewProjection[2][3] - viewProjection[2][1];
        m_Planes[Top].Distance = viewProjection[3][3] - viewProjection[3][1];

        // Near plane
        m_Planes[Near].Normal.x = viewProjection[0][3] + viewProjection[0][2];
        m_Planes[Near].Normal.y = viewProjection[1][3] + viewProjection[1][2];
        m_Planes[Near].Normal.z = viewProjection[2][3] + viewProjection[2][2];
        m_Planes[Near].Distance = viewProjection[3][3] + viewProjection[3][2];

        // Far plane
        m_Planes[Far].Normal.x = viewProjection[0][3] - viewProjection[0][2];
        m_Planes[Far].Normal.y = viewProjection[1][3] - viewProjection[1][2];
        m_Planes[Far].Normal.z = viewProjection[2][3] - viewProjection[2][2];
        m_Planes[Far].Distance = viewProjection[3][3] - viewProjection[3][2];

        // Normalize all planes
        for (auto& plane : m_Planes) {
            plane.Normalize();
        }
    }

    // Test if a point is inside the frustum
    bool IsPointVisible(const glm::vec3& point) const {
        for (const auto& plane : m_Planes) {
            if (plane.DistanceToPoint(point) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    // Test if a sphere is visible (fully or partially)
    bool IsSphereVisible(const glm::vec3& center, f32 radius) const {
        for (const auto& plane : m_Planes) {
            if (plane.DistanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    // Test if a sphere is visible
    bool IsSphereVisible(const BoundingSphere& sphere) const {
        return IsSphereVisible(sphere.Center, sphere.Radius);
    }

    // Test if an AABB is visible (fully or partially)
    bool IsBoxVisible(const AABB& box) const {
        for (const auto& plane : m_Planes) {
            // Find the corner most aligned with plane normal (positive vertex)
            glm::vec3 positiveVertex = box.Min;
            if (plane.Normal.x >= 0.0f) positiveVertex.x = box.Max.x;
            if (plane.Normal.y >= 0.0f) positiveVertex.y = box.Max.y;
            if (plane.Normal.z >= 0.0f) positiveVertex.z = box.Max.z;

            // If the positive vertex is behind the plane, box is outside
            if (plane.DistanceToPoint(positiveVertex) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    // Test if a transformed AABB is visible
    bool IsBoxVisible(const AABB& localBox, const glm::mat4& transform) const {
        AABB worldBox = localBox.Transform(transform);
        return IsBoxVisible(worldBox);
    }

    const Plane& GetPlane(PlaneIndex index) const { return m_Planes[index]; }

private:
    std::array<Plane, PlaneIndex::Count> m_Planes;
};

} // namespace Engine
