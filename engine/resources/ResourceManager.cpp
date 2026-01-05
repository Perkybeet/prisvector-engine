#include "resources/ResourceManager.hpp"
#include "core/Logger.hpp"

namespace Engine {

ResourceManager& ResourceManager::Instance() {
    static ResourceManager instance;
    return instance;
}

String ResourceManager::ResolvePath(const String& relativePath) const {
    if (m_BasePath.empty() || relativePath.empty()) {
        return relativePath;
    }
    if (relativePath[0] == '/' || (relativePath.size() > 1 && relativePath[1] == ':')) {
        return relativePath;
    }
    String path = m_BasePath;
    if (path.back() != '/' && path.back() != '\\') {
        path += '/';
    }
    return path + relativePath;
}

// Texture management
Ref<Texture2D> ResourceManager::LoadTexture(const String& name, const String& filepath, bool flipVertically) {
    if (auto it = m_Textures.find(name); it != m_Textures.end()) {
        LOG_CORE_WARN("Texture '{}' already loaded, returning cached version", name);
        return it->second;
    }

    String fullPath = ResolvePath(filepath);
    auto texture = CreateRef<Texture2D>(fullPath, flipVertically);

    if (!texture->IsLoaded()) {
        LOG_CORE_ERROR("Failed to load texture: {}", fullPath);
        return nullptr;
    }

    m_Textures[name] = texture;
    LOG_CORE_INFO("Loaded texture: '{}' from {}", name, fullPath);
    return texture;
}

Ref<Texture2D> ResourceManager::GetTexture(const String& name) {
    if (auto it = m_Textures.find(name); it != m_Textures.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::HasTexture(const String& name) const {
    return m_Textures.find(name) != m_Textures.end();
}

void ResourceManager::UnloadTexture(const String& name) {
    if (auto it = m_Textures.find(name); it != m_Textures.end()) {
        m_Textures.erase(it);
        LOG_CORE_INFO("Unloaded texture: '{}'", name);
    }
}

// Mesh management
Ref<Mesh> ResourceManager::LoadMesh(const String& name, const String& filepath, const MeshLoadOptions& options) {
    if (auto it = m_Meshes.find(name); it != m_Meshes.end()) {
        LOG_CORE_WARN("Mesh '{}' already loaded, returning cached version", name);
        return it->second;
    }

    String fullPath = ResolvePath(filepath);
    auto mesh = MeshLoader::LoadOBJ(fullPath, options);

    if (!mesh) {
        LOG_CORE_ERROR("Failed to load mesh: {}", fullPath);
        return nullptr;
    }

    mesh->Upload();
    m_Meshes[name] = mesh;
    LOG_CORE_INFO("Loaded mesh: '{}' from {}", name, fullPath);
    return mesh;
}

Ref<Mesh> ResourceManager::GetMesh(const String& name) {
    if (auto it = m_Meshes.find(name); it != m_Meshes.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::HasMesh(const String& name) const {
    return m_Meshes.find(name) != m_Meshes.end();
}

void ResourceManager::UnloadMesh(const String& name) {
    if (auto it = m_Meshes.find(name); it != m_Meshes.end()) {
        m_Meshes.erase(it);
        LOG_CORE_INFO("Unloaded mesh: '{}'", name);
    }
}

// Primitive meshes
Ref<Mesh> ResourceManager::GetCube() {
    if (!m_CubeMesh) {
        m_CubeMesh = MeshLoader::CreateCube(1.0f);
        m_CubeMesh->Upload();
    }
    return m_CubeMesh;
}

Ref<Mesh> ResourceManager::GetSphere(u32 segments, u32 rings) {
    u64 key = static_cast<u64>(segments) | (static_cast<u64>(rings) << 32);
    if (auto it = m_SphereMeshes.find(key); it != m_SphereMeshes.end()) {
        return it->second;
    }

    auto mesh = MeshLoader::CreateSphere(1.0f, segments, rings);
    mesh->Upload();
    m_SphereMeshes[key] = mesh;
    return mesh;
}

Ref<Mesh> ResourceManager::GetPlane(u32 subdivisions) {
    if (auto it = m_PlaneMeshes.find(subdivisions); it != m_PlaneMeshes.end()) {
        return it->second;
    }

    auto mesh = MeshLoader::CreatePlane(1.0f, 1.0f, subdivisions);
    mesh->Upload();
    m_PlaneMeshes[subdivisions] = mesh;
    return mesh;
}

Ref<Mesh> ResourceManager::GetCylinder(u32 segments) {
    if (auto it = m_CylinderMeshes.find(segments); it != m_CylinderMeshes.end()) {
        return it->second;
    }

    auto mesh = MeshLoader::CreateCylinder(0.5f, 1.0f, segments);
    mesh->Upload();
    m_CylinderMeshes[segments] = mesh;
    return mesh;
}

// Shader management
Ref<Shader> ResourceManager::LoadShader(const String& name, const String& filepath) {
    if (auto it = m_Shaders.find(name); it != m_Shaders.end()) {
        LOG_CORE_WARN("Shader '{}' already loaded, returning cached version", name);
        return it->second;
    }

    String fullPath = ResolvePath(filepath);
    auto shader = CreateRef<Shader>(fullPath);

    m_Shaders[name] = shader;
    LOG_CORE_INFO("Loaded shader: '{}' from {}", name, fullPath);
    return shader;
}

Ref<Shader> ResourceManager::GetShader(const String& name) {
    if (auto it = m_Shaders.find(name); it != m_Shaders.end()) {
        return it->second;
    }
    return nullptr;
}

bool ResourceManager::HasShader(const String& name) const {
    return m_Shaders.find(name) != m_Shaders.end();
}

void ResourceManager::UnloadShader(const String& name) {
    if (auto it = m_Shaders.find(name); it != m_Shaders.end()) {
        m_Shaders.erase(it);
        LOG_CORE_INFO("Unloaded shader: '{}'", name);
    }
}

// General management
void ResourceManager::Clear() {
    m_Textures.clear();
    m_Meshes.clear();
    m_Shaders.clear();
    m_CubeMesh.reset();
    m_SphereMeshes.clear();
    m_PlaneMeshes.clear();
    m_CylinderMeshes.clear();
    LOG_CORE_INFO("ResourceManager cleared");
}

void ResourceManager::UnloadUnused() {
    u32 unloaded = 0;

    for (auto it = m_Textures.begin(); it != m_Textures.end();) {
        if (it->second.use_count() == 1) {
            it = m_Textures.erase(it);
            ++unloaded;
        } else {
            ++it;
        }
    }

    for (auto it = m_Meshes.begin(); it != m_Meshes.end();) {
        if (it->second.use_count() == 1) {
            it = m_Meshes.erase(it);
            ++unloaded;
        } else {
            ++it;
        }
    }

    for (auto it = m_Shaders.begin(); it != m_Shaders.end();) {
        if (it->second.use_count() == 1) {
            it = m_Shaders.erase(it);
            ++unloaded;
        } else {
            ++it;
        }
    }

    if (unloaded > 0) {
        LOG_CORE_INFO("Unloaded {} unused resources", unloaded);
    }
}

ResourceManager::Stats ResourceManager::GetStats() const {
    Stats stats;
    stats.TexturesLoaded = static_cast<u32>(m_Textures.size());
    stats.MeshesLoaded = static_cast<u32>(m_Meshes.size() +
        (m_CubeMesh ? 1 : 0) +
        m_SphereMeshes.size() +
        m_PlaneMeshes.size() +
        m_CylinderMeshes.size());
    stats.ShadersLoaded = static_cast<u32>(m_Shaders.size());
    return stats;
}

} // namespace Engine
