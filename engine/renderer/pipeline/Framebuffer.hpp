#pragma once

#include "core/Types.hpp"
#include "renderer/Texture.hpp"
#include <glm/glm.hpp>

namespace Engine {

enum class FramebufferTextureFormat {
    None = 0,

    // Color formats
    RGBA8,
    RGBA16F,
    RGBA32F,
    RG16F,
    R11G11B10F,

    // Depth/Stencil
    Depth24Stencil8,
    Depth32F
};

struct FramebufferTextureSpecification {
    FramebufferTextureFormat Format = FramebufferTextureFormat::None;
    TextureFilter MinFilter = TextureFilter::Linear;
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureWrap WrapS = TextureWrap::ClampToEdge;
    TextureWrap WrapT = TextureWrap::ClampToEdge;

    FramebufferTextureSpecification() = default;
    FramebufferTextureSpecification(FramebufferTextureFormat format) : Format(format) {}
};

struct FramebufferAttachmentSpecification {
    Vector<FramebufferTextureSpecification> Attachments;

    FramebufferAttachmentSpecification() = default;
    FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> list)
        : Attachments(list) {}
};

struct FramebufferSpecification {
    u32 Width = 1280;
    u32 Height = 720;
    FramebufferAttachmentSpecification Attachments;
    u32 Samples = 1;
    bool SwapChainTarget = false;
};

class Framebuffer {
public:
    Framebuffer(const FramebufferSpecification& spec);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void Bind();
    void Unbind();

    void Resize(u32 width, u32 height);
    void Invalidate();

    u32 GetColorAttachmentRendererID(u32 index = 0) const;
    u32 GetDepthAttachmentRendererID() const { return m_DepthAttachment; }

    void BindColorTexture(u32 attachmentIndex, u32 slot);
    void BindDepthTexture(u32 slot);

    void ClearColorAttachment(u32 index, const glm::vec4& value);
    void ClearDepthAttachment(f32 value = 1.0f);
    void Clear(const glm::vec4& clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), f32 depthValue = 1.0f);

    const FramebufferSpecification& GetSpecification() const { return m_Specification; }
    u32 GetWidth() const { return m_Specification.Width; }
    u32 GetHeight() const { return m_Specification.Height; }
    u32 GetColorAttachmentCount() const { return static_cast<u32>(m_ColorAttachments.size()); }
    u32 GetRendererID() const { return m_RendererID; }

    static void BindDefault();
    static void BlitFramebuffer(const Framebuffer& src, u32 dstFBO,
                                 u32 srcWidth, u32 srcHeight,
                                 u32 dstWidth, u32 dstHeight);

private:
    void CreateAttachments();
    void DeleteAttachments();

private:
    u32 m_RendererID = 0;
    FramebufferSpecification m_Specification;

    Vector<FramebufferTextureSpecification> m_ColorAttachmentSpecs;
    FramebufferTextureSpecification m_DepthAttachmentSpec;

    Vector<u32> m_ColorAttachments;
    u32 m_DepthAttachment = 0;
};

} // namespace Engine
