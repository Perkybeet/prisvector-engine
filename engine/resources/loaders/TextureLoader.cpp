#include "resources/loaders/TextureLoader.hpp"
#include "core/Logger.hpp"
#include <filesystem>

namespace Engine {

HashMap<String, Ref<Texture2D>> TextureLoader::s_TextureCache;
Ref<Texture2D> TextureLoader::s_DefaultWhite;
Ref<Texture2D> TextureLoader::s_DefaultBlack;
Ref<Texture2D> TextureLoader::s_DefaultNormal;

Ref<Texture2D> TextureLoader::Load(const String& filepath, bool flipVertically) {
    // Check cache first
    auto it = s_TextureCache.find(filepath);
    if (it != s_TextureCache.end()) {
        return it->second;
    }

    // Load new texture
    auto texture = CreateRef<Texture2D>(filepath, flipVertically);

    if (texture->IsLoaded()) {
        s_TextureCache[filepath] = texture;
        LOG_CORE_DEBUG("TextureLoader: Cached texture '{}'", filepath);
    } else {
        LOG_CORE_WARN("TextureLoader: Failed to load '{}'", filepath);
    }

    return texture;
}

PBRTextureSet TextureLoader::LoadPBRSet(const String& directory) {
    namespace fs = std::filesystem;
    PBRTextureSet set;

    // Common PBR texture naming conventions
    const Vector<String> albedoNames = {"albedo", "basecolor", "diffuse", "color"};
    const Vector<String> normalNames = {"normal", "normalmap", "norm"};
    const Vector<String> metallicNames = {"metallic", "metal", "metalness"};
    const Vector<String> roughnessNames = {"roughness", "rough"};
    const Vector<String> aoNames = {"ao", "ambient_occlusion", "ambientocclusion", "occlusion"};
    const Vector<String> emissiveNames = {"emissive", "emission", "glow"};
    const Vector<String> extensions = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};

    auto findTexture = [&](const Vector<String>& names) -> String {
        for (const auto& name : names) {
            for (const auto& ext : extensions) {
                String path = directory + "/" + name + ext;
                if (fs::exists(path)) {
                    return path;
                }
                // Try uppercase
                String upperName = name;
                upperName[0] = static_cast<char>(std::toupper(upperName[0]));
                path = directory + "/" + upperName + ext;
                if (fs::exists(path)) {
                    return path;
                }
            }
        }
        return "";
    };

    // Load each texture type
    String albedoPath = findTexture(albedoNames);
    if (!albedoPath.empty()) {
        set.Albedo = Load(albedoPath);
    }

    String normalPath = findTexture(normalNames);
    if (!normalPath.empty()) {
        set.Normal = Load(normalPath, false);  // Don't flip normals
    }

    String metallicPath = findTexture(metallicNames);
    if (!metallicPath.empty()) {
        set.Metallic = Load(metallicPath);
    }

    String roughnessPath = findTexture(roughnessNames);
    if (!roughnessPath.empty()) {
        set.Roughness = Load(roughnessPath);
    }

    String aoPath = findTexture(aoNames);
    if (!aoPath.empty()) {
        set.AO = Load(aoPath);
    }

    String emissivePath = findTexture(emissiveNames);
    if (!emissivePath.empty()) {
        set.Emissive = Load(emissivePath);
    }

    LOG_CORE_INFO("TextureLoader: Loaded PBR set from '{}' (albedo:{}, normal:{}, metallic:{}, rough:{}, ao:{}, emissive:{})",
                  directory,
                  set.HasAlbedo() ? "yes" : "no",
                  set.HasNormal() ? "yes" : "no",
                  set.HasMetallic() ? "yes" : "no",
                  set.HasRoughness() ? "yes" : "no",
                  set.HasAO() ? "yes" : "no",
                  set.HasEmissive() ? "yes" : "no");

    return set;
}

PBRTextureSet TextureLoader::LoadPBRSet(
    const String& albedoPath,
    const String& normalPath,
    const String& metallicPath,
    const String& roughnessPath,
    const String& aoPath,
    const String& emissivePath
) {
    PBRTextureSet set;

    if (!albedoPath.empty()) {
        set.Albedo = Load(albedoPath);
    }
    if (!normalPath.empty()) {
        set.Normal = Load(normalPath, false);
    }
    if (!metallicPath.empty()) {
        set.Metallic = Load(metallicPath);
    }
    if (!roughnessPath.empty()) {
        set.Roughness = Load(roughnessPath);
    }
    if (!aoPath.empty()) {
        set.AO = Load(aoPath);
    }
    if (!emissivePath.empty()) {
        set.Emissive = Load(emissivePath);
    }

    return set;
}

Ref<Texture2D> TextureLoader::CreateSolidColor(u32 r, u32 g, u32 b, u32 a) {
    TextureSpecification spec;
    spec.Width = 1;
    spec.Height = 1;
    spec.Format = TextureFormat::RGBA8;
    spec.GenerateMipmaps = false;
    spec.MinFilter = TextureFilter::Nearest;
    spec.MagFilter = TextureFilter::Nearest;

    auto texture = CreateRef<Texture2D>(spec);

    u8 data[4] = {
        static_cast<u8>(r),
        static_cast<u8>(g),
        static_cast<u8>(b),
        static_cast<u8>(a)
    };
    texture->SetData(data, sizeof(data));

    return texture;
}

Ref<Texture2D> TextureLoader::CreateSolidColor(const glm::vec4& color) {
    return CreateSolidColor(
        static_cast<u32>(color.r * 255.0f),
        static_cast<u32>(color.g * 255.0f),
        static_cast<u32>(color.b * 255.0f),
        static_cast<u32>(color.a * 255.0f)
    );
}

Ref<Texture2D> TextureLoader::GetDefaultWhite() {
    if (!s_DefaultWhite) {
        s_DefaultWhite = CreateSolidColor(255, 255, 255, 255);
        LOG_CORE_DEBUG("TextureLoader: Created default white texture");
    }
    return s_DefaultWhite;
}

Ref<Texture2D> TextureLoader::GetDefaultBlack() {
    if (!s_DefaultBlack) {
        s_DefaultBlack = CreateSolidColor(0, 0, 0, 255);
        LOG_CORE_DEBUG("TextureLoader: Created default black texture");
    }
    return s_DefaultBlack;
}

Ref<Texture2D> TextureLoader::GetDefaultNormal() {
    if (!s_DefaultNormal) {
        // Flat normal pointing up (128, 128, 255) = (0, 0, 1) in tangent space
        s_DefaultNormal = CreateSolidColor(128, 128, 255, 255);
        LOG_CORE_DEBUG("TextureLoader: Created default normal texture");
    }
    return s_DefaultNormal;
}

void TextureLoader::ClearCache() {
    size_t count = s_TextureCache.size();
    s_TextureCache.clear();
    s_DefaultWhite.reset();
    s_DefaultBlack.reset();
    s_DefaultNormal.reset();
    LOG_CORE_INFO("TextureLoader: Cleared cache ({} textures)", count);
}

size_t TextureLoader::GetCacheSize() {
    return s_TextureCache.size();
}

bool TextureLoader::IsLoaded(const String& filepath) {
    return s_TextureCache.find(filepath) != s_TextureCache.end();
}

} // namespace Engine
