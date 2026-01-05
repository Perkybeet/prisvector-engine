#pragma once

#include "core/Types.hpp"
#include "renderer/Mesh.hpp"

namespace Engine {

struct MeshLoadOptions {
    bool FlipUVs = false;
    bool GenerateNormals = false;
    bool GenerateTangents = true;
    bool CalculateBounds = true;
};

class MeshLoader {
public:
    static Ref<Mesh> LoadOBJ(const String& filepath, const MeshLoadOptions& options = {});

    static Ref<Mesh> CreateCube(float size = 1.0f);
    static Ref<Mesh> CreateSphere(float radius = 1.0f, u32 segments = 32, u32 rings = 16);
    static Ref<Mesh> CreatePlane(float width = 1.0f, float height = 1.0f, u32 subdivisions = 1);
    static Ref<Mesh> CreateCylinder(float radius = 0.5f, float height = 1.0f, u32 segments = 32);

private:
    struct OBJData {
        Vector<glm::vec3> Positions;
        Vector<glm::vec3> Normals;
        Vector<glm::vec2> TexCoords;

        struct Face {
            struct Index {
                i32 Position = -1;
                i32 TexCoord = -1;
                i32 Normal = -1;
            };
            Vector<Index> Indices;
        };
        Vector<Face> Faces;
        String MaterialLib;
        String CurrentMaterial;
    };

    static bool ParseOBJ(const String& filepath, OBJData& data);
    static Ref<Mesh> ConvertOBJToMesh(const OBJData& data, const MeshLoadOptions& options);
};

} // namespace Engine
