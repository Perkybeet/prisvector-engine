#include "renderer/Mesh.hpp"
#include "core/Logger.hpp"
#include <limits>

namespace Engine {

Mesh::Mesh(const Vector<Vertex>& vertices, const Vector<u32>& indices)
    : m_Vertices(vertices), m_Indices(indices) {
    RecalculateBounds();
}

Mesh::Mesh(Vector<Vertex>&& vertices, Vector<u32>&& indices)
    : m_Vertices(std::move(vertices)), m_Indices(std::move(indices)) {
    RecalculateBounds();
}

void Mesh::Upload() {
    if (m_Vertices.empty()) {
        LOG_CORE_WARN("Attempting to upload empty mesh");
        return;
    }

    m_VAO = CreateRef<VertexArray>();

    m_VBO = CreateRef<VertexBuffer>(
        reinterpret_cast<const f32*>(m_Vertices.data()),
        static_cast<u32>(m_Vertices.size() * sizeof(Vertex))
    );
    m_VBO->SetLayout(GetDefaultLayout());

    m_VAO->AddVertexBuffer(m_VBO);

    if (!m_Indices.empty()) {
        m_IBO = CreateRef<IndexBuffer>(
            m_Indices.data(),
            static_cast<u32>(m_Indices.size())
        );
        m_VAO->SetIndexBuffer(m_IBO);
    }

    LOG_CORE_INFO("Uploaded mesh '{}': {} vertices, {} indices",
                  m_Name.empty() ? "unnamed" : m_Name,
                  m_Vertices.size(), m_Indices.size());
}

void Mesh::Bind() const {
    if (m_VAO) {
        m_VAO->Bind();
    }
}

void Mesh::Unbind() const {
    if (m_VAO) {
        m_VAO->Unbind();
    }
}

void Mesh::RecalculateBounds() {
    if (m_Vertices.empty()) {
        m_Bounds = AABB();
        m_BoundingSphere = BoundingSphere();
        return;
    }

    glm::vec3 minPoint(std::numeric_limits<float>::max());
    glm::vec3 maxPoint(std::numeric_limits<float>::lowest());

    for (const auto& vertex : m_Vertices) {
        minPoint = glm::min(minPoint, vertex.Position);
        maxPoint = glm::max(maxPoint, vertex.Position);
    }

    m_Bounds = AABB(minPoint, maxPoint);

    glm::vec3 center = m_Bounds.GetCenter();
    float maxDistSq = 0.0f;
    for (const auto& vertex : m_Vertices) {
        float distSq = glm::dot(vertex.Position - center, vertex.Position - center);
        maxDistSq = std::max(maxDistSq, distSq);
    }
    m_BoundingSphere.Center = center;
    m_BoundingSphere.Radius = std::sqrt(maxDistSq);
}

void Mesh::RecalculateNormals() {
    for (auto& vertex : m_Vertices) {
        vertex.Normal = glm::vec3(0.0f);
    }

    for (size_t i = 0; i + 2 < m_Indices.size(); i += 3) {
        u32 i0 = m_Indices[i];
        u32 i1 = m_Indices[i + 1];
        u32 i2 = m_Indices[i + 2];

        glm::vec3 v0 = m_Vertices[i0].Position;
        glm::vec3 v1 = m_Vertices[i1].Position;
        glm::vec3 v2 = m_Vertices[i2].Position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        m_Vertices[i0].Normal += normal;
        m_Vertices[i1].Normal += normal;
        m_Vertices[i2].Normal += normal;
    }

    for (auto& vertex : m_Vertices) {
        float len = glm::length(vertex.Normal);
        if (len > 0.0f) {
            vertex.Normal /= len;
        }
    }
}

void Mesh::RecalculateTangents() {
    for (auto& vertex : m_Vertices) {
        vertex.Tangent = glm::vec3(0.0f);
        vertex.Bitangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i + 2 < m_Indices.size(); i += 3) {
        u32 i0 = m_Indices[i];
        u32 i1 = m_Indices[i + 1];
        u32 i2 = m_Indices[i + 2];

        Vertex& v0 = m_Vertices[i0];
        Vertex& v1 = m_Vertices[i1];
        Vertex& v2 = m_Vertices[i2];

        glm::vec3 edge1 = v1.Position - v0.Position;
        glm::vec3 edge2 = v2.Position - v0.Position;

        glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

        float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        if (std::abs(det) < 1e-6f) {
            continue;
        }
        float f = 1.0f / det;

        glm::vec3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;

        v0.Bitangent += bitangent;
        v1.Bitangent += bitangent;
        v2.Bitangent += bitangent;
    }

    for (auto& vertex : m_Vertices) {
        float len = glm::length(vertex.Tangent);
        if (len > 0.0f) {
            vertex.Tangent /= len;
        }
        len = glm::length(vertex.Bitangent);
        if (len > 0.0f) {
            vertex.Bitangent /= len;
        }
    }
}

BufferLayout Mesh::GetDefaultLayout() {
    return BufferLayout{
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float3, "a_Normal" },
        { ShaderDataType::Float2, "a_TexCoords" },
        { ShaderDataType::Float3, "a_Tangent" },
        { ShaderDataType::Float3, "a_Bitangent" }
    };
}

} // namespace Engine
