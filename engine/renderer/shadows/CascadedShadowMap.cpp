#include "renderer/shadows/CascadedShadowMap.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine {

CascadedShadowMap::CascadedShadowMap(u32 resolution)
    : m_Resolution(resolution) {
    CreateResources();
}

CascadedShadowMap::~CascadedShadowMap() {
    DeleteResources();
}

CascadedShadowMap::CascadedShadowMap(CascadedShadowMap&& other) noexcept
    : m_Resolution(other.m_Resolution)
    , m_DepthTextureArray(other.m_DepthTextureArray)
    , m_Framebuffer(other.m_Framebuffer)
    , m_Cascades(std::move(other.m_Cascades))
    , m_SplitDistances(std::move(other.m_SplitDistances))
    , m_Initialized(other.m_Initialized) {
    other.m_DepthTextureArray = 0;
    other.m_Framebuffer = 0;
    other.m_Initialized = false;
}

CascadedShadowMap& CascadedShadowMap::operator=(CascadedShadowMap&& other) noexcept {
    if (this != &other) {
        DeleteResources();

        m_Resolution = other.m_Resolution;
        m_DepthTextureArray = other.m_DepthTextureArray;
        m_Framebuffer = other.m_Framebuffer;
        m_Cascades = std::move(other.m_Cascades);
        m_SplitDistances = std::move(other.m_SplitDistances);
        m_Initialized = other.m_Initialized;

        other.m_DepthTextureArray = 0;
        other.m_Framebuffer = 0;
        other.m_Initialized = false;
    }
    return *this;
}

void CascadedShadowMap::CreateResources() {
    // Create 2D texture array for cascades (depth-only)
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_DepthTextureArray);
    glTextureStorage3D(m_DepthTextureArray, 1, GL_DEPTH_COMPONENT32F,
                       m_Resolution, m_Resolution, CSM_CASCADE_COUNT);

    // Texture parameters for depth comparison
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Set border color to 1.0 (no shadow outside the map)
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(m_DepthTextureArray, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Enable hardware depth comparison for shadow sampling
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_DepthTextureArray, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // Create framebuffer
    glCreateFramebuffers(1, &m_Framebuffer);

    // We'll attach layers dynamically when rendering each cascade
    glNamedFramebufferDrawBuffer(m_Framebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(m_Framebuffer, GL_NONE);

    m_Initialized = true;

    LOG_CORE_DEBUG("Created CSM with resolution {}x{} ({} cascades)",
                   m_Resolution, m_Resolution, CSM_CASCADE_COUNT);
}

void CascadedShadowMap::DeleteResources() {
    if (m_Framebuffer) {
        glDeleteFramebuffers(1, &m_Framebuffer);
        m_Framebuffer = 0;
    }

    if (m_DepthTextureArray) {
        glDeleteTextures(1, &m_DepthTextureArray);
        m_DepthTextureArray = 0;
    }

    m_Initialized = false;
}

void CascadedShadowMap::Resize(u32 resolution) {
    if (resolution == m_Resolution) return;

    m_Resolution = resolution;
    DeleteResources();
    CreateResources();
}

void CascadedShadowMap::Update(const Camera& camera,
                                const glm::vec3& lightDirection,
                                f32 maxDistance,
                                f32 lambda) {
    if (!m_Initialized) return;

    // Extract near plane from camera projection matrix
    // For perspective: P[2][3] = -2*near*far/(far-near), P[2][2] = -(far+near)/(far-near)
    // near = P[3][2] / (P[2][2] - 1)
    const glm::mat4& proj = camera.GetProjectionMatrix();
    f32 nearPlane = proj[3][2] / (proj[2][2] - 1.0f);

    CalculateCascadeSplits(nearPlane, maxDistance, lambda);
    CalculateCascadeMatrices(camera, glm::normalize(lightDirection));
}

void CascadedShadowMap::CalculateCascadeSplits(f32 nearPlane, f32 maxDistance, f32 lambda) {
    // Practical split scheme: blend between logarithmic and linear splits
    // lambda = 0: linear, lambda = 1: logarithmic

    m_SplitDistances[0] = nearPlane;

    for (u32 i = 1; i <= CSM_CASCADE_COUNT; ++i) {
        f32 p = static_cast<f32>(i) / static_cast<f32>(CSM_CASCADE_COUNT);

        f32 logSplit = nearPlane * std::pow(maxDistance / nearPlane, p);
        f32 linSplit = nearPlane + (maxDistance - nearPlane) * p;

        m_SplitDistances[i] = lambda * logSplit + (1.0f - lambda) * linSplit;
    }
}

std::array<glm::vec3, 8> CascadedShadowMap::GetFrustumCornersWorldSpace(
    const Camera& camera, f32 nearPlane, f32 farPlane) const {

    // Get inverse view-projection to transform NDC corners to world space
    const glm::mat4& view = camera.GetViewMatrix();
    const glm::mat4& proj = camera.GetProjectionMatrix();

    // Create a projection matrix for this specific near/far range
    // Extract FOV and aspect from original projection
    f32 tanHalfFov = 1.0f / proj[1][1];
    f32 aspect = proj[1][1] / proj[0][0];

    glm::mat4 cascadeProj = glm::perspective(
        2.0f * std::atan(tanHalfFov),
        aspect,
        nearPlane,
        farPlane
    );

    glm::mat4 invViewProj = glm::inverse(cascadeProj * view);

    std::array<glm::vec3, 8> corners;
    u32 cornerIndex = 0;

    // NDC cube corners
    for (u32 z = 0; z < 2; ++z) {
        for (u32 y = 0; y < 2; ++y) {
            for (u32 x = 0; x < 2; ++x) {
                glm::vec4 pt = invViewProj * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f
                );
                corners[cornerIndex++] = glm::vec3(pt) / pt.w;
            }
        }
    }

    return corners;
}

void CascadedShadowMap::CalculateCascadeMatrices(const Camera& camera,
                                                  const glm::vec3& lightDir) {
    for (u32 cascade = 0; cascade < CSM_CASCADE_COUNT; ++cascade) {
        f32 nearSplit = m_SplitDistances[cascade];
        f32 farSplit = m_SplitDistances[cascade + 1];

        auto corners = GetFrustumCornersWorldSpace(camera, nearSplit, farSplit);

        // Calculate frustum center
        glm::vec3 center(0.0f);
        for (const auto& corner : corners) {
            center += corner;
        }
        center /= 8.0f;

        // Create light view matrix looking at frustum center
        glm::mat4 lightView = glm::lookAt(
            center - lightDir * 100.0f,  // Light position (far away in light direction)
            center,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Transform frustum corners to light space and find bounding box
        f32 minX = std::numeric_limits<f32>::max();
        f32 maxX = std::numeric_limits<f32>::lowest();
        f32 minY = std::numeric_limits<f32>::max();
        f32 maxY = std::numeric_limits<f32>::lowest();
        f32 minZ = std::numeric_limits<f32>::max();
        f32 maxZ = std::numeric_limits<f32>::lowest();

        for (const auto& corner : corners) {
            glm::vec4 lightSpaceCorner = lightView * glm::vec4(corner, 1.0f);
            minX = std::min(minX, lightSpaceCorner.x);
            maxX = std::max(maxX, lightSpaceCorner.x);
            minY = std::min(minY, lightSpaceCorner.y);
            maxY = std::max(maxY, lightSpaceCorner.y);
            minZ = std::min(minZ, lightSpaceCorner.z);
            maxZ = std::max(maxZ, lightSpaceCorner.z);
        }

        // Extend Z range to include shadow casters behind the camera frustum
        constexpr f32 zMultiplier = 5.0f;
        if (minZ < 0) {
            minZ *= zMultiplier;
        } else {
            minZ /= zMultiplier;
        }
        if (maxZ < 0) {
            maxZ /= zMultiplier;
        } else {
            maxZ *= zMultiplier;
        }

        // Create orthographic projection
        glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        // Stabilize the shadow map to prevent swimming/shimmer
        glm::mat4 lightViewProj = lightProjection * lightView;
        lightViewProj = StabilizeProjection(lightViewProj, m_Resolution);

        // Store cascade info
        m_Cascades[cascade].ViewMatrix = lightView;
        m_Cascades[cascade].ProjectionMatrix = lightProjection;
        m_Cascades[cascade].ViewProjectionMatrix = lightViewProj;
        m_Cascades[cascade].SplitNear = nearSplit;
        m_Cascades[cascade].SplitFar = farSplit;

        // Calculate world-space AABB for frustum culling
        glm::mat4 invLightViewProj = glm::inverse(lightViewProj);
        glm::vec3 worldMin(std::numeric_limits<f32>::max());
        glm::vec3 worldMax(std::numeric_limits<f32>::lowest());

        for (u32 z = 0; z < 2; ++z) {
            for (u32 y = 0; y < 2; ++y) {
                for (u32 x = 0; x < 2; ++x) {
                    glm::vec4 pt = invLightViewProj * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f
                    );
                    glm::vec3 worldPt = glm::vec3(pt) / pt.w;
                    worldMin = glm::min(worldMin, worldPt);
                    worldMax = glm::max(worldMax, worldPt);
                }
            }
        }
        m_Cascades[cascade].WorldBounds = AABB(worldMin, worldMax);
    }
}

glm::mat4 CascadedShadowMap::StabilizeProjection(const glm::mat4& lightViewProj,
                                                   u32 resolution) const {
    // Quantize the projection to texel boundaries to prevent shadow swimming

    // Transform origin to shadow map space
    glm::vec4 shadowOrigin = lightViewProj * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin *= static_cast<f32>(resolution) * 0.5f;

    // Calculate offset needed to snap to texel
    glm::vec2 roundedOrigin = glm::round(glm::vec2(shadowOrigin));
    glm::vec2 offset = roundedOrigin - glm::vec2(shadowOrigin);
    offset /= static_cast<f32>(resolution) * 0.5f;

    // Apply offset to projection matrix
    glm::mat4 stabilized = lightViewProj;
    stabilized[3][0] += offset.x;
    stabilized[3][1] += offset.y;

    return stabilized;
}

void CascadedShadowMap::BindForCascade(u32 cascadeIndex) {
    if (cascadeIndex >= CSM_CASCADE_COUNT) {
        LOG_CORE_ERROR("Cascade index {} out of range", cascadeIndex);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    // Attach specific layer of texture array
    glNamedFramebufferTextureLayer(m_Framebuffer, GL_DEPTH_ATTACHMENT,
                                    m_DepthTextureArray, 0, cascadeIndex);

    glViewport(0, 0, m_Resolution, m_Resolution);

    // Enable depth testing and writing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Disable color writing
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Optional: Polygon offset to reduce shadow acne
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1f, 4.0f);
}

void CascadedShadowMap::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Restore state
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void CascadedShadowMap::Clear() {
    f32 clearDepth = 1.0f;
    for (u32 i = 0; i < CSM_CASCADE_COUNT; ++i) {
        glNamedFramebufferTextureLayer(m_Framebuffer, GL_DEPTH_ATTACHMENT,
                                        m_DepthTextureArray, 0, i);
        glClearNamedFramebufferfv(m_Framebuffer, GL_DEPTH, 0, &clearDepth);
    }
}

void CascadedShadowMap::ClearCascade(u32 cascadeIndex) {
    if (cascadeIndex >= CSM_CASCADE_COUNT) return;

    f32 clearDepth = 1.0f;
    glNamedFramebufferTextureLayer(m_Framebuffer, GL_DEPTH_ATTACHMENT,
                                    m_DepthTextureArray, 0, cascadeIndex);
    glClearNamedFramebufferfv(m_Framebuffer, GL_DEPTH, 0, &clearDepth);
}

void CascadedShadowMap::BindTexture(u32 slot) const {
    glBindTextureUnit(slot, m_DepthTextureArray);
}

const CascadeInfo& CascadedShadowMap::GetCascadeInfo(u32 index) const {
    if (index >= CSM_CASCADE_COUNT) {
        LOG_CORE_ERROR("Cascade index {} out of range", index);
        static CascadeInfo empty;
        return empty;
    }
    return m_Cascades[index];
}

void CascadedShadowMap::FillGPUData(GPUCascadedShadowData& outData,
                                     f32 softness, f32 bias, f32 normalBias) const {
    f32 texelSize = 1.0f / static_cast<f32>(m_Resolution);

    for (u32 i = 0; i < CSM_CASCADE_COUNT; ++i) {
        outData.Cascades[i].ViewProjection = m_Cascades[i].ViewProjectionMatrix;
        outData.Cascades[i].SplitDepthBias = glm::vec4(
            m_SplitDistances[i + 1],    // Split depth (far plane of cascade)
            texelSize,
            bias,
            normalBias
        );
        outData.CascadeSplitDepths[i] = m_SplitDistances[i + 1];
    }

    outData.ShadowParams = glm::vec4(
        softness,
        m_SplitDistances[CSM_CASCADE_COUNT],    // Max shadow distance
        m_SplitDistances[CSM_CASCADE_COUNT] * 0.9f,  // Fade start
        1.0f    // Enabled
    );
}

} // namespace Engine
