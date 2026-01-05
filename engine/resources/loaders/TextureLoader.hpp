#pragma once

#include "core/Types.hpp"
#include "renderer/Texture.hpp"

namespace Engine {

// PBR texture set for a material
struct PBRTextureSet {
    Ref<Texture2D> Albedo;
    Ref<Texture2D> Normal;
    Ref<Texture2D> Metallic;
    Ref<Texture2D> Roughness;
    Ref<Texture2D> AO;
    Ref<Texture2D> Emissive;

    bool HasAlbedo() const { return Albedo && Albedo->IsLoaded(); }
    bool HasNormal() const { return Normal && Normal->IsLoaded(); }
    bool HasMetallic() const { return Metallic && Metallic->IsLoaded(); }
    bool HasRoughness() const { return Roughness && Roughness->IsLoaded(); }
    bool HasAO() const { return AO && AO->IsLoaded(); }
    bool HasEmissive() const { return Emissive && Emissive->IsLoaded(); }
};

class TextureLoader {
public:
    // Load a texture (cached - returns existing if already loaded)
    static Ref<Texture2D> Load(const String& filepath, bool flipVertically = true);

    // Load a PBR texture set from a directory
    // Expects files named: albedo.png, normal.png, metallic.png, roughness.png, ao.png, emissive.png
    static PBRTextureSet LoadPBRSet(const String& directory);

    // Load a PBR texture set with custom filenames
    static PBRTextureSet LoadPBRSet(
        const String& albedoPath,
        const String& normalPath = "",
        const String& metallicPath = "",
        const String& roughnessPath = "",
        const String& aoPath = "",
        const String& emissivePath = ""
    );

    // Create solid color textures (useful for defaults)
    static Ref<Texture2D> CreateSolidColor(u32 r, u32 g, u32 b, u32 a = 255);
    static Ref<Texture2D> CreateSolidColor(const glm::vec4& color);

    // Default textures (1x1 pixel fallbacks)
    static Ref<Texture2D> GetDefaultWhite();
    static Ref<Texture2D> GetDefaultBlack();
    static Ref<Texture2D> GetDefaultNormal();   // (128, 128, 255) - flat normal

    // Cache management
    static void ClearCache();
    static size_t GetCacheSize();
    static bool IsLoaded(const String& filepath);

private:
    static HashMap<String, Ref<Texture2D>> s_TextureCache;
    static Ref<Texture2D> s_DefaultWhite;
    static Ref<Texture2D> s_DefaultBlack;
    static Ref<Texture2D> s_DefaultNormal;
};

} // namespace Engine
