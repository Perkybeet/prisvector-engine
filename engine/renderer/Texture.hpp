#pragma once

#include "core/Types.hpp"
#include <glm/glm.hpp>

namespace Engine {

enum class TextureFormat {
    None = 0,
    R8,
    RG8,
    RGB8,
    RGBA8,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,
    Depth24Stencil8
};

enum class TextureFilter {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureWrap {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

struct TextureSpecification {
    u32 Width = 1;
    u32 Height = 1;
    TextureFormat Format = TextureFormat::RGBA8;
    TextureFilter MinFilter = TextureFilter::Linear;
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureWrap WrapS = TextureWrap::Repeat;
    TextureWrap WrapT = TextureWrap::Repeat;
    bool GenerateMipmaps = true;
    glm::vec4 BorderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

class Texture2D {
public:
    Texture2D(const TextureSpecification& spec);
    Texture2D(const String& filepath, bool flipVertically = true);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void Bind(u32 slot = 0) const;
    void Unbind() const;

    void SetData(const void* data, u32 size);

    u32 GetWidth() const { return m_Width; }
    u32 GetHeight() const { return m_Height; }
    u32 GetRendererID() const { return m_RendererID; }
    TextureFormat GetFormat() const { return m_Format; }
    const String& GetFilePath() const { return m_FilePath; }

    bool IsLoaded() const { return m_IsLoaded; }

    bool operator==(const Texture2D& other) const {
        return m_RendererID == other.m_RendererID;
    }

private:
    void CreateTexture(const TextureSpecification& spec);
    void SetFilterAndWrap(const TextureSpecification& spec);

private:
    u32 m_RendererID = 0;
    u32 m_Width = 0;
    u32 m_Height = 0;
    TextureFormat m_Format = TextureFormat::None;
    String m_FilePath;
    bool m_IsLoaded = false;
};

} // namespace Engine
