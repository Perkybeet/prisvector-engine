#include "renderer/pipeline/GBuffer.hpp"

#include <glad/gl.h>

namespace Engine {

GBuffer::GBuffer(u32 width, u32 height) {
    FramebufferSpecification spec;
    spec.Width = width;
    spec.Height = height;
    spec.Attachments = {
        // RT0: Position (RGB) + Linear Depth (A)
        FramebufferTextureFormat::RGBA16F,
        // RT1: Normal (RGB) + Metallic (A)
        FramebufferTextureFormat::RGBA16F,
        // RT2: Albedo (RGB) + Roughness (A)
        FramebufferTextureFormat::RGBA8,
        // RT3: Emission (RGB) + AO (A)
        FramebufferTextureFormat::RGBA8,
        // Depth + Stencil
        FramebufferTextureFormat::Depth24Stencil8
    };

    m_Framebuffer = CreateScope<Framebuffer>(spec);
}

void GBuffer::Bind() {
    m_Framebuffer->Bind();
}

void GBuffer::Unbind() {
    m_Framebuffer->Unbind();
}

void GBuffer::Resize(u32 width, u32 height) {
    m_Framebuffer->Resize(width, height);
}

void GBuffer::Clear() {
    m_Framebuffer->ClearColorAttachment(Position, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    m_Framebuffer->ClearColorAttachment(Normal, glm::vec4(0.5f, 0.5f, 0.5f, 0.0f));
    m_Framebuffer->ClearColorAttachment(Albedo, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    m_Framebuffer->ClearColorAttachment(Emission, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    m_Framebuffer->ClearDepthAttachment(1.0f);
}

void GBuffer::BindTextures(u32 startSlot) {
    m_Framebuffer->BindColorTexture(Position, startSlot);
    m_Framebuffer->BindColorTexture(Normal, startSlot + 1);
    m_Framebuffer->BindColorTexture(Albedo, startSlot + 2);
    m_Framebuffer->BindColorTexture(Emission, startSlot + 3);
}

void GBuffer::BindPositionTexture(u32 slot) {
    m_Framebuffer->BindColorTexture(Position, slot);
}

void GBuffer::BindNormalTexture(u32 slot) {
    m_Framebuffer->BindColorTexture(Normal, slot);
}

void GBuffer::BindAlbedoTexture(u32 slot) {
    m_Framebuffer->BindColorTexture(Albedo, slot);
}

void GBuffer::BindEmissionTexture(u32 slot) {
    m_Framebuffer->BindColorTexture(Emission, slot);
}

void GBuffer::BindDepthTexture(u32 slot) {
    m_Framebuffer->BindDepthTexture(slot);
}

u32 GBuffer::GetPositionTextureID() const {
    return m_Framebuffer->GetColorAttachmentRendererID(Position);
}

u32 GBuffer::GetNormalTextureID() const {
    return m_Framebuffer->GetColorAttachmentRendererID(Normal);
}

u32 GBuffer::GetAlbedoTextureID() const {
    return m_Framebuffer->GetColorAttachmentRendererID(Albedo);
}

u32 GBuffer::GetEmissionTextureID() const {
    return m_Framebuffer->GetColorAttachmentRendererID(Emission);
}

u32 GBuffer::GetDepthTextureID() const {
    return m_Framebuffer->GetDepthAttachmentRendererID();
}

} // namespace Engine
