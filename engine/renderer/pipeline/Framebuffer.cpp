#include "renderer/pipeline/Framebuffer.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>

namespace Engine {

namespace {

GLenum FramebufferTextureFormatToGL(FramebufferTextureFormat format) {
    switch (format) {
        case FramebufferTextureFormat::RGBA8:           return GL_RGBA8;
        case FramebufferTextureFormat::RGBA16F:         return GL_RGBA16F;
        case FramebufferTextureFormat::RGBA32F:         return GL_RGBA32F;
        case FramebufferTextureFormat::RG16F:           return GL_RG16F;
        case FramebufferTextureFormat::R11G11B10F:      return GL_R11F_G11F_B10F;
        case FramebufferTextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case FramebufferTextureFormat::Depth32F:        return GL_DEPTH_COMPONENT32F;
        default: return GL_RGBA8;
    }
}

GLenum TextureFilterToGL(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear:  return GL_LINEAR;
        default: return GL_LINEAR;
    }
}

GLenum TextureWrapToGL(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        default: return GL_CLAMP_TO_EDGE;
    }
}

bool IsDepthFormat(FramebufferTextureFormat format) {
    switch (format) {
        case FramebufferTextureFormat::Depth24Stencil8:
        case FramebufferTextureFormat::Depth32F:
            return true;
        default:
            return false;
    }
}

GLenum GetDepthAttachmentType(FramebufferTextureFormat format) {
    switch (format) {
        case FramebufferTextureFormat::Depth24Stencil8: return GL_DEPTH_STENCIL_ATTACHMENT;
        case FramebufferTextureFormat::Depth32F:        return GL_DEPTH_ATTACHMENT;
        default: return GL_DEPTH_ATTACHMENT;
    }
}

} // namespace

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec) {
    for (const auto& attachment : spec.Attachments.Attachments) {
        if (IsDepthFormat(attachment.Format)) {
            m_DepthAttachmentSpec = attachment;
        } else {
            m_ColorAttachmentSpecs.push_back(attachment);
        }
    }

    Invalidate();
}

Framebuffer::~Framebuffer() {
    DeleteAttachments();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_RendererID(other.m_RendererID)
    , m_Specification(std::move(other.m_Specification))
    , m_ColorAttachmentSpecs(std::move(other.m_ColorAttachmentSpecs))
    , m_DepthAttachmentSpec(other.m_DepthAttachmentSpec)
    , m_ColorAttachments(std::move(other.m_ColorAttachments))
    , m_DepthAttachment(other.m_DepthAttachment) {
    other.m_RendererID = 0;
    other.m_DepthAttachment = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        DeleteAttachments();

        m_RendererID = other.m_RendererID;
        m_Specification = std::move(other.m_Specification);
        m_ColorAttachmentSpecs = std::move(other.m_ColorAttachmentSpecs);
        m_DepthAttachmentSpec = other.m_DepthAttachmentSpec;
        m_ColorAttachments = std::move(other.m_ColorAttachments);
        m_DepthAttachment = other.m_DepthAttachment;

        other.m_RendererID = 0;
        other.m_DepthAttachment = 0;
    }
    return *this;
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Specification.Width, m_Specification.Height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::BindDefault() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(u32 width, u32 height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        LOG_CORE_WARN("Invalid framebuffer size: {}x{}", width, height);
        return;
    }

    m_Specification.Width = width;
    m_Specification.Height = height;

    Invalidate();
}

void Framebuffer::Invalidate() {
    if (m_RendererID) {
        DeleteAttachments();
    }

    glCreateFramebuffers(1, &m_RendererID);

    CreateAttachments();

    if (m_ColorAttachments.size() > 1) {
        if (m_ColorAttachments.size() > 8) {
            LOG_CORE_ERROR("Too many color attachments (max 8)");
            return;
        }

        GLenum buffers[8];
        for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
            buffers[i] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
        }
        glNamedFramebufferDrawBuffers(m_RendererID,
                                       static_cast<GLsizei>(m_ColorAttachments.size()),
                                       buffers);
    } else if (m_ColorAttachments.empty()) {
        glNamedFramebufferDrawBuffer(m_RendererID, GL_NONE);
    }

    GLenum status = glCheckNamedFramebufferStatus(m_RendererID, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_CORE_ERROR("Framebuffer is not complete! Status: 0x{:X}", status);
    }
}

void Framebuffer::CreateAttachments() {
    bool multisample = m_Specification.Samples > 1;
    GLenum textureTarget = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

    // Color attachments
    if (!m_ColorAttachmentSpecs.empty()) {
        m_ColorAttachments.resize(m_ColorAttachmentSpecs.size());
        glCreateTextures(textureTarget, static_cast<GLsizei>(m_ColorAttachments.size()),
                         m_ColorAttachments.data());

        for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
            const auto& spec = m_ColorAttachmentSpecs[i];

            if (multisample) {
                glTextureStorage2DMultisample(
                    m_ColorAttachments[i],
                    m_Specification.Samples,
                    FramebufferTextureFormatToGL(spec.Format),
                    m_Specification.Width,
                    m_Specification.Height,
                    GL_TRUE
                );
            } else {
                glTextureStorage2D(
                    m_ColorAttachments[i],
                    1,
                    FramebufferTextureFormatToGL(spec.Format),
                    m_Specification.Width,
                    m_Specification.Height
                );

                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MIN_FILTER,
                                    TextureFilterToGL(spec.MinFilter));
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_MAG_FILTER,
                                    TextureFilterToGL(spec.MagFilter));
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_S,
                                    TextureWrapToGL(spec.WrapS));
                glTextureParameteri(m_ColorAttachments[i], GL_TEXTURE_WRAP_T,
                                    TextureWrapToGL(spec.WrapT));
            }

            glNamedFramebufferTexture(m_RendererID,
                                       GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                                       m_ColorAttachments[i],
                                       0);
        }
    }

    // Depth attachment
    if (m_DepthAttachmentSpec.Format != FramebufferTextureFormat::None) {
        glCreateTextures(textureTarget, 1, &m_DepthAttachment);

        if (multisample) {
            glTextureStorage2DMultisample(
                m_DepthAttachment,
                m_Specification.Samples,
                FramebufferTextureFormatToGL(m_DepthAttachmentSpec.Format),
                m_Specification.Width,
                m_Specification.Height,
                GL_TRUE
            );
        } else {
            glTextureStorage2D(
                m_DepthAttachment,
                1,
                FramebufferTextureFormatToGL(m_DepthAttachmentSpec.Format),
                m_Specification.Width,
                m_Specification.Height
            );

            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_DepthAttachment, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glNamedFramebufferTexture(m_RendererID,
                                   GetDepthAttachmentType(m_DepthAttachmentSpec.Format),
                                   m_DepthAttachment,
                                   0);
    }
}

void Framebuffer::DeleteAttachments() {
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        m_RendererID = 0;
    }

    if (!m_ColorAttachments.empty()) {
        glDeleteTextures(static_cast<GLsizei>(m_ColorAttachments.size()),
                         m_ColorAttachments.data());
        m_ColorAttachments.clear();
    }

    if (m_DepthAttachment) {
        glDeleteTextures(1, &m_DepthAttachment);
        m_DepthAttachment = 0;
    }
}

u32 Framebuffer::GetColorAttachmentRendererID(u32 index) const {
    if (index >= m_ColorAttachments.size()) {
        LOG_CORE_ERROR("Color attachment index out of range: {}", index);
        return 0;
    }
    return m_ColorAttachments[index];
}

void Framebuffer::BindColorTexture(u32 attachmentIndex, u32 slot) {
    if (attachmentIndex >= m_ColorAttachments.size()) {
        LOG_CORE_ERROR("Color attachment index out of range: {}", attachmentIndex);
        return;
    }
    glBindTextureUnit(slot, m_ColorAttachments[attachmentIndex]);
}

void Framebuffer::BindDepthTexture(u32 slot) {
    if (m_DepthAttachment) {
        glBindTextureUnit(slot, m_DepthAttachment);
    }
}

void Framebuffer::ClearColorAttachment(u32 index, const glm::vec4& value) {
    if (index >= m_ColorAttachments.size()) {
        LOG_CORE_ERROR("Color attachment index out of range: {}", index);
        return;
    }
    glClearNamedFramebufferfv(m_RendererID, GL_COLOR, index, &value[0]);
}

void Framebuffer::ClearDepthAttachment(f32 value) {
    glClearNamedFramebufferfv(m_RendererID, GL_DEPTH, 0, &value);
}

void Framebuffer::Clear(const glm::vec4& clearColor, f32 depthValue) {
    for (size_t i = 0; i < m_ColorAttachments.size(); ++i) {
        glClearNamedFramebufferfv(m_RendererID, GL_COLOR, static_cast<GLint>(i), &clearColor[0]);
    }

    if (m_DepthAttachment) {
        if (m_DepthAttachmentSpec.Format == FramebufferTextureFormat::Depth24Stencil8) {
            GLint stencilValue = 0;
            glClearNamedFramebufferfi(m_RendererID, GL_DEPTH_STENCIL, 0, depthValue, stencilValue);
        } else {
            glClearNamedFramebufferfv(m_RendererID, GL_DEPTH, 0, &depthValue);
        }
    }
}

void Framebuffer::BlitFramebuffer(const Framebuffer& src, u32 dstFBO,
                                   u32 srcWidth, u32 srcHeight,
                                   u32 dstWidth, u32 dstHeight) {
    glBlitNamedFramebuffer(
        src.m_RendererID, dstFBO,
        0, 0, srcWidth, srcHeight,
        0, 0, dstWidth, dstHeight,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR
    );
}

} // namespace Engine
