#include "Ray.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <limits>
#include <cmath>

namespace Engine {

bool Ray::IntersectsAABB(const AABB& aabb, f32& tMin) const {
    // Slab method for ray-AABB intersection
    glm::vec3 invDir;
    invDir.x = (std::abs(Direction.x) > 1e-6f) ? 1.0f / Direction.x : 1e6f;
    invDir.y = (std::abs(Direction.y) > 1e-6f) ? 1.0f / Direction.y : 1e6f;
    invDir.z = (std::abs(Direction.z) > 1e-6f) ? 1.0f / Direction.z : 1e6f;

    glm::vec3 t1 = (aabb.Min - Origin) * invDir;
    glm::vec3 t2 = (aabb.Max - Origin) * invDir;

    glm::vec3 tMinVec = glm::min(t1, t2);
    glm::vec3 tMaxVec = glm::max(t1, t2);

    f32 tNear = std::max({tMinVec.x, tMinVec.y, tMinVec.z});
    f32 tFar = std::min({tMaxVec.x, tMaxVec.y, tMaxVec.z});

    // No intersection if ray misses or is behind
    if (tNear > tFar || tFar < 0.0f) {
        return false;
    }

    // Return the closest positive intersection
    tMin = tNear >= 0.0f ? tNear : tFar;
    return true;
}

Ray Ray::FromScreenPoint(
    const glm::vec2& screenPos,
    const glm::vec2& viewportPos,
    const glm::vec2& viewportSize,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix)
{
    // Convert screen position to viewport-relative coordinates
    glm::vec2 viewportMousePos = screenPos - viewportPos;

    // Convert to normalized device coordinates (-1 to 1)
    glm::vec2 ndc;
    ndc.x = (2.0f * viewportMousePos.x) / viewportSize.x - 1.0f;
    ndc.y = 1.0f - (2.0f * viewportMousePos.y) / viewportSize.y;  // Y is flipped

    // Create ray endpoints in clip space
    glm::vec4 rayClipNear(ndc.x, ndc.y, -1.0f, 1.0f);
    glm::vec4 rayClipFar(ndc.x, ndc.y, 1.0f, 1.0f);

    // Transform to view space
    glm::mat4 invProj = glm::inverse(projectionMatrix);
    glm::vec4 rayViewNear = invProj * rayClipNear;
    glm::vec4 rayViewFar = invProj * rayClipFar;
    rayViewNear /= rayViewNear.w;
    rayViewFar /= rayViewFar.w;

    // Transform to world space
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 rayWorldNear = glm::vec3(invView * rayViewNear);
    glm::vec3 rayWorldFar = glm::vec3(invView * rayViewFar);

    glm::vec3 direction = glm::normalize(rayWorldFar - rayWorldNear);

    return Ray(rayWorldNear, direction);
}

} // namespace Engine
