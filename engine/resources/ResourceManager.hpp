#pragma once

#include "core/Types.hpp"
#include "renderer/Texture.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "resources/loaders/MeshLoader.hpp"

namespace Engine {

class ResourceManager {
public:
    static ResourceManager& Instance();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Texture management
    Ref<Texture2D> LoadTexture(const String& name, const String& filepath, bool flipVertically = true);
    Ref<Texture2D> GetTexture(const String& name);
    bool HasTexture(const String& name) const;
    void UnloadTexture(const String& name);

    // Mesh management
    Ref<Mesh> LoadMesh(const String& name, const String& filepath, const MeshLoadOptions& options = {});
    Ref<Mesh> GetMesh(const String& name);
    bool HasMesh(const String& name) const;
    void UnloadMesh(const String& name);

    // Primitive meshes (cached automatically)
    Ref<Mesh> GetCube();
    Ref<Mesh> GetSphere(u32 segments = 32, u32 rings = 16);
    Ref<Mesh> GetPlane(u32 subdivisions = 1);
    Ref<Mesh> GetCylinder(u32 segments = 32);

    // Shader management
    Ref<Shader> LoadShader(const String& name, const String& filepath);
    Ref<Shader> GetShader(const String& name);
    bool HasShader(const String& name) const;
    void UnloadShader(const String& name);

    // General management
    void Clear();
    void UnloadUnused();

    // Statistics
    struct Stats {
        u32 TexturesLoaded = 0;
        u32 MeshesLoaded = 0;
        u32 ShadersLoaded = 0;
        size_t EstimatedMemory = 0;
    };
    Stats GetStats() const;

    void SetBasePath(const String& path) { m_BasePath = path; }
    const String& GetBasePath() const { return m_BasePath; }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    String ResolvePath(const String& relativePath) const;

private:
    HashMap<String, Ref<Texture2D>> m_Textures;
    HashMap<String, Ref<Mesh>> m_Meshes;
    HashMap<String, Ref<Shader>> m_Shaders;

    // Primitive cache
    Ref<Mesh> m_CubeMesh;
    HashMap<u64, Ref<Mesh>> m_SphereMeshes;  // key = segments | (rings << 32)
    HashMap<u32, Ref<Mesh>> m_PlaneMeshes;   // key = subdivisions
    HashMap<u32, Ref<Mesh>> m_CylinderMeshes; // key = segments

    String m_BasePath;
};

} // namespace Engine
