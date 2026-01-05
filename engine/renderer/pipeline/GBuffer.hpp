#pragma once

#include "core/Types.hpp"
#include "renderer/pipeline/Framebuffer.hpp"

namespace Engine {

// G-Buffer layout optimized for PBR Deferred Rendering:
// RT0 (RGBA16F): Position.xyz + Linear Depth
// RT1 (RGBA16F): Normal.xyz (world space, encoded) + Metallic
// RT2 (RGBA8):   Albedo.rgb + Roughness
// RT3 (RGBA8):   Emission.rgb + AO
// Depth: Depth24Stencil8

class GBuffer {
public:
    enum Attachment : u32 {
        Position = 0,
        Normal = 1,
        Albedo = 2,
        Emission = 3,
        Count = 4
    };

    GBuffer(u32 width, u32 height);
    ~GBuffer() = default;

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    GBuffer(GBuffer&&) = default;
    GBuffer& operator=(GBuffer&&) = default;

    void Bind();
    void Unbind();
    void Resize(u32 width, u32 height);

    void Clear();

    void BindTextures(u32 startSlot = 0);
    void BindPositionTexture(u32 slot);
    void BindNormalTexture(u32 slot);
    void BindAlbedoTexture(u32 slot);
    void BindEmissionTexture(u32 slot);
    void BindDepthTexture(u32 slot);

    u32 GetPositionTextureID() const;
    u32 GetNormalTextureID() const;
    u32 GetAlbedoTextureID() const;
    u32 GetEmissionTextureID() const;
    u32 GetDepthTextureID() const;

    u32 GetWidth() const { return m_Framebuffer->GetWidth(); }
    u32 GetHeight() const { return m_Framebuffer->GetHeight(); }

    Framebuffer& GetFramebuffer() { return *m_Framebuffer; }
    const Framebuffer& GetFramebuffer() const { return *m_Framebuffer; }

private:
    Scope<Framebuffer> m_Framebuffer;
};

} // namespace Engine
