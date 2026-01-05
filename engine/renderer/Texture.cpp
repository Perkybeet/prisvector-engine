#include "renderer/Texture.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Engine {

namespace {

GLenum TextureFormatToGL(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:              return GL_R8;
        case TextureFormat::RG8:             return GL_RG8;
        case TextureFormat::RGB8:            return GL_RGB8;
        case TextureFormat::RGBA8:           return GL_RGBA8;
        case TextureFormat::R16F:            return GL_R16F;
        case TextureFormat::RG16F:           return GL_RG16F;
        case TextureFormat::RGB16F:          return GL_RGB16F;
        case TextureFormat::RGBA16F:         return GL_RGBA16F;
        case TextureFormat::R32F:            return GL_R32F;
        case TextureFormat::RG32F:           return GL_RG32F;
        case TextureFormat::RGB32F:          return GL_RGB32F;
        case TextureFormat::RGBA32F:         return GL_RGBA32F;
        case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        default: return GL_RGBA8;
    }
}

GLenum TextureFormatToBaseFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:
            return GL_RED;
        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:
            return GL_RG;
        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:
            return GL_RGB;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:
            return GL_RGBA;
        case TextureFormat::Depth24Stencil8:
            return GL_DEPTH_STENCIL;
        default:
            return GL_RGBA;
    }
}

GLenum TextureFormatToDataType(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::RG8:
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
            return GL_UNSIGNED_BYTE;
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
        case TextureFormat::R32F:
        case TextureFormat::RG32F:
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
            return GL_FLOAT;
        case TextureFormat::Depth24Stencil8:
            return GL_UNSIGNED_INT_24_8;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

GLenum TextureFilterToGL(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest:              return GL_NEAREST;
        case TextureFilter::Linear:               return GL_LINEAR;
        case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilter::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilter::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
        case TextureFilter::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
        default: return GL_LINEAR;
    }
}

GLenum TextureWrapToGL(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
        default: return GL_REPEAT;
    }
}

u32 GetChannelCount(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8:
        case TextureFormat::R16F:
        case TextureFormat::R32F:
            return 1;
        case TextureFormat::RG8:
        case TextureFormat::RG16F:
        case TextureFormat::RG32F:
            return 2;
        case TextureFormat::RGB8:
        case TextureFormat::RGB16F:
        case TextureFormat::RGB32F:
            return 3;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA16F:
        case TextureFormat::RGBA32F:
            return 4;
        default:
            return 4;
    }
}

} // namespace

Texture2D::Texture2D(const TextureSpecification& spec)
    : m_Width(spec.Width), m_Height(spec.Height), m_Format(spec.Format) {
    CreateTexture(spec);
    m_IsLoaded = true;
}

Texture2D::Texture2D(const String& filepath, bool flipVertically)
    : m_FilePath(filepath) {
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

    int width, height, channels;
    stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
        LOG_CORE_ERROR("Failed to load texture: {}", filepath);
        LOG_CORE_ERROR("stb_image error: {}", stbi_failure_reason());
        return;
    }

    m_Width = static_cast<u32>(width);
    m_Height = static_cast<u32>(height);

    switch (channels) {
        case 1: m_Format = TextureFormat::R8; break;
        case 2: m_Format = TextureFormat::RG8; break;
        case 3: m_Format = TextureFormat::RGB8; break;
        case 4: m_Format = TextureFormat::RGBA8; break;
        default:
            LOG_CORE_ERROR("Unsupported channel count: {}", channels);
            stbi_image_free(data);
            return;
    }

    TextureSpecification spec;
    spec.Width = m_Width;
    spec.Height = m_Height;
    spec.Format = m_Format;
    spec.GenerateMipmaps = true;
    spec.MinFilter = TextureFilter::LinearMipmapLinear;
    spec.MagFilter = TextureFilter::Linear;

    CreateTexture(spec);

    glTextureSubImage2D(
        m_RendererID, 0, 0, 0,
        m_Width, m_Height,
        TextureFormatToBaseFormat(m_Format),
        GL_UNSIGNED_BYTE,
        data
    );

    if (spec.GenerateMipmaps) {
        glGenerateTextureMipmap(m_RendererID);
    }

    stbi_image_free(data);
    m_IsLoaded = true;

    LOG_CORE_INFO("Loaded texture: {} ({}x{}, {} channels)",
                     filepath, m_Width, m_Height, channels);
}

Texture2D::~Texture2D() {
    if (m_RendererID) {
        glDeleteTextures(1, &m_RendererID);
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_RendererID(other.m_RendererID)
    , m_Width(other.m_Width)
    , m_Height(other.m_Height)
    , m_Format(other.m_Format)
    , m_FilePath(std::move(other.m_FilePath))
    , m_IsLoaded(other.m_IsLoaded) {
    other.m_RendererID = 0;
    other.m_IsLoaded = false;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_RendererID) {
            glDeleteTextures(1, &m_RendererID);
        }

        m_RendererID = other.m_RendererID;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Format = other.m_Format;
        m_FilePath = std::move(other.m_FilePath);
        m_IsLoaded = other.m_IsLoaded;

        other.m_RendererID = 0;
        other.m_IsLoaded = false;
    }
    return *this;
}

void Texture2D::Bind(u32 slot) const {
    glBindTextureUnit(slot, m_RendererID);
}

void Texture2D::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::SetData(const void* data, u32 size) {
    u32 bpp = GetChannelCount(m_Format);
    if (size != m_Width * m_Height * bpp) {
        LOG_CORE_ERROR("Data size must match texture dimensions!");
        return;
    }

    glTextureSubImage2D(
        m_RendererID, 0, 0, 0,
        m_Width, m_Height,
        TextureFormatToBaseFormat(m_Format),
        TextureFormatToDataType(m_Format),
        data
    );
}

void Texture2D::CreateTexture(const TextureSpecification& spec) {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);

    u32 mipLevels = 1;
    if (spec.GenerateMipmaps) {
        mipLevels = static_cast<u32>(std::floor(std::log2(std::max(spec.Width, spec.Height)))) + 1;
    }

    glTextureStorage2D(
        m_RendererID,
        mipLevels,
        TextureFormatToGL(spec.Format),
        spec.Width,
        spec.Height
    );

    SetFilterAndWrap(spec);
}

void Texture2D::SetFilterAndWrap(const TextureSpecification& spec) {
    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, TextureFilterToGL(spec.MinFilter));
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, TextureFilterToGL(spec.MagFilter));
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, TextureWrapToGL(spec.WrapS));
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, TextureWrapToGL(spec.WrapT));

    if (spec.WrapS == TextureWrap::ClampToBorder || spec.WrapT == TextureWrap::ClampToBorder) {
        glTextureParameterfv(m_RendererID, GL_TEXTURE_BORDER_COLOR, &spec.BorderColor[0]);
    }
}

} // namespace Engine
