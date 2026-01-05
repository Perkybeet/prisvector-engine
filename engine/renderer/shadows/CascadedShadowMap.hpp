#pragma once

#include "core/Types.hpp"
#include "renderer/shadows/ShadowTypes.hpp"
#include "camera/Camera.hpp"
#include <glm/glm.hpp>
#include <array>

namespace Engine {

class CascadedShadowMap {
public:
    explicit CascadedShadowMap(u32 resolution = DEFAULT_CSM_RESOLUTION);
    ~CascadedShadowMap();

    // Non-copyable, movable
    CascadedShadowMap(const CascadedShadowMap&) = delete;
    CascadedShadowMap& operator=(const CascadedShadowMap&) = delete;
    CascadedShadowMap(CascadedShadowMap&& other) noexcept;
    CascadedShadowMap& operator=(CascadedShadowMap&& other) noexcept;

    // Resize all cascade layers
    void Resize(u32 resolution);

    // Calculate cascade splits and matrices for current frame
    void Update(const Camera& camera,
                const glm::vec3& lightDirection,
                f32 maxDistance,
                f32 lambda);

    // Bind framebuffer for rendering a specific cascade
    void BindForCascade(u32 cascadeIndex);
    void Unbind();
    void Clear();
    void ClearCascade(u32 cascadeIndex);

    // Bind shadow map texture array for sampling in shaders
    void BindTexture(u32 slot) const;
    u32 GetTextureID() const { return m_DepthTextureArray; }
    u32 GetFramebufferID() const { return m_Framebuffer; }

    // Cascade info accessors
    const CascadeInfo& GetCascadeInfo(u32 index) const;
    const std::array<CascadeInfo, CSM_CASCADE_COUNT>& GetAllCascades() const { return m_Cascades; }
    f32 GetSplitDistance(u32 index) const { return m_SplitDistances[index]; }

    // Fill GPU data structure for UBO upload
    void FillGPUData(GPUCascadedShadowData& outData, f32 softness, f32 bias, f32 normalBias) const;

    u32 GetResolution() const { return m_Resolution; }

private:
    void CreateResources();
    void DeleteResources();

    void CalculateCascadeSplits(f32 nearPlane, f32 maxDistance, f32 lambda);
    void CalculateCascadeMatrices(const Camera& camera, const glm::vec3& lightDir);

    // Stabilize projection to prevent shadow swimming/shimmer
    glm::mat4 StabilizeProjection(const glm::mat4& lightViewProj, u32 resolution) const;

    // Calculate frustum corners in world space for a given near/far range
    std::array<glm::vec3, 8> GetFrustumCornersWorldSpace(const Camera& camera, f32 nearPlane, f32 farPlane) const;

private:
    u32 m_Resolution;

    // OpenGL resources
    u32 m_DepthTextureArray = 0;        // GL_TEXTURE_2D_ARRAY
    u32 m_Framebuffer = 0;

    // Per-cascade data
    std::array<CascadeInfo, CSM_CASCADE_COUNT> m_Cascades;
    std::array<f32, CSM_CASCADE_COUNT + 1> m_SplitDistances;

    bool m_Initialized = false;
};

} // namespace Engine
