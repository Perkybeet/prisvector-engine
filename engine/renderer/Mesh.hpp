#pragma once

#include "core/Types.hpp"
#include "math/AABB.hpp"
#include "renderer/opengl/GLBuffer.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include <glm/glm.hpp>

namespace Engine {

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct SubMesh {
    u32 BaseVertex = 0;
    u32 BaseIndex = 0;
    u32 IndexCount = 0;
    u32 MaterialIndex = 0;
    String Name;
};

class Mesh {
public:
    Mesh() = default;
    Mesh(const Vector<Vertex>& vertices, const Vector<u32>& indices);
    Mesh(Vector<Vertex>&& vertices, Vector<u32>&& indices);
    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;

    void Upload();
    void Bind() const;
    void Unbind() const;

    const Ref<VertexArray>& GetVertexArray() const { return m_VAO; }
    u32 GetIndexCount() const { return static_cast<u32>(m_Indices.size()); }
    u32 GetVertexCount() const { return static_cast<u32>(m_Vertices.size()); }

    const AABB& GetBounds() const { return m_Bounds; }
    const BoundingSphere& GetBoundingSphere() const { return m_BoundingSphere; }

    const Vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
    void AddSubMesh(const SubMesh& submesh) { m_SubMeshes.push_back(submesh); }

    const String& GetName() const { return m_Name; }
    void SetName(const String& name) { m_Name = name; }
    const String& GetFilePath() const { return m_FilePath; }
    void SetFilePath(const String& path) { m_FilePath = path; }

    bool IsUploaded() const { return m_VAO != nullptr; }

    void RecalculateBounds();
    void RecalculateNormals();
    void RecalculateTangents();

    static BufferLayout GetDefaultLayout();

private:
    Vector<Vertex> m_Vertices;
    Vector<u32> m_Indices;
    Vector<SubMesh> m_SubMeshes;

    Ref<VertexArray> m_VAO;
    Ref<VertexBuffer> m_VBO;
    Ref<IndexBuffer> m_IBO;

    AABB m_Bounds;
    BoundingSphere m_BoundingSphere;

    String m_Name;
    String m_FilePath;
};

} // namespace Engine
