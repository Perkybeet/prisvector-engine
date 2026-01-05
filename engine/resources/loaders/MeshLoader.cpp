#include "resources/loaders/MeshLoader.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace Engine {

namespace {

struct VertexKey {
    i32 p, t, n;
    bool operator==(const VertexKey& other) const {
        return p == other.p && t == other.t && n == other.n;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        return std::hash<i32>()(k.p) ^ (std::hash<i32>()(k.t) << 1) ^ (std::hash<i32>()(k.n) << 2);
    }
};

Vector<String> Split(const String& str, char delimiter) {
    Vector<String> tokens;
    std::istringstream stream(str);
    String token;
    while (std::getline(stream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

String Trim(const String& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == String::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace

bool MeshLoader::ParseOBJ(const String& filepath, OBJData& data) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_CORE_ERROR("Failed to open OBJ file: {}", filepath);
        return false;
    }

    String line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        String prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            data.Positions.push_back(pos);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            data.Normals.push_back(normal);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            data.TexCoords.push_back(uv);
        }
        else if (prefix == "f") {
            OBJData::Face face;
            String vertexStr;
            while (iss >> vertexStr) {
                OBJData::Face::Index idx;
                auto parts = Split(vertexStr, '/');

                if (parts.size() >= 1 && !parts[0].empty()) {
                    idx.Position = std::stoi(parts[0]) - 1;
                }
                if (parts.size() >= 2 && !parts[1].empty()) {
                    idx.TexCoord = std::stoi(parts[1]) - 1;
                }
                if (parts.size() >= 3 && !parts[2].empty()) {
                    idx.Normal = std::stoi(parts[2]) - 1;
                }

                face.Indices.push_back(idx);
            }
            if (face.Indices.size() >= 3) {
                data.Faces.push_back(face);
            }
        }
        else if (prefix == "mtllib") {
            iss >> data.MaterialLib;
        }
        else if (prefix == "usemtl") {
            iss >> data.CurrentMaterial;
        }
    }

    return true;
}

Ref<Mesh> MeshLoader::ConvertOBJToMesh(const OBJData& data, const MeshLoadOptions& options) {
    Vector<Vertex> vertices;
    Vector<u32> indices;
    std::unordered_map<VertexKey, u32, VertexKeyHash> vertexMap;

    for (const auto& face : data.Faces) {
        Vector<u32> faceIndices;

        for (const auto& idx : face.Indices) {
            VertexKey key{idx.Position, idx.TexCoord, idx.Normal};

            auto it = vertexMap.find(key);
            if (it != vertexMap.end()) {
                faceIndices.push_back(it->second);
            } else {
                Vertex vertex{};

                if (idx.Position >= 0 && idx.Position < static_cast<i32>(data.Positions.size())) {
                    vertex.Position = data.Positions[idx.Position];
                }

                if (idx.Normal >= 0 && idx.Normal < static_cast<i32>(data.Normals.size())) {
                    vertex.Normal = data.Normals[idx.Normal];
                }

                if (idx.TexCoord >= 0 && idx.TexCoord < static_cast<i32>(data.TexCoords.size())) {
                    vertex.TexCoords = data.TexCoords[idx.TexCoord];
                    if (options.FlipUVs) {
                        vertex.TexCoords.y = 1.0f - vertex.TexCoords.y;
                    }
                }

                u32 newIndex = static_cast<u32>(vertices.size());
                vertices.push_back(vertex);
                vertexMap[key] = newIndex;
                faceIndices.push_back(newIndex);
            }
        }

        for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[i]);
            indices.push_back(faceIndices[i + 1]);
        }
    }

    auto mesh = CreateRef<Mesh>(std::move(vertices), std::move(indices));

    if (options.GenerateNormals || data.Normals.empty()) {
        mesh->RecalculateNormals();
    }

    if (options.GenerateTangents) {
        mesh->RecalculateTangents();
    }

    if (options.CalculateBounds) {
        mesh->RecalculateBounds();
    }

    return mesh;
}

Ref<Mesh> MeshLoader::LoadOBJ(const String& filepath, const MeshLoadOptions& options) {
    OBJData data;
    if (!ParseOBJ(filepath, data)) {
        return nullptr;
    }

    if (data.Positions.empty() || data.Faces.empty()) {
        LOG_CORE_ERROR("OBJ file has no geometry: {}", filepath);
        return nullptr;
    }

    auto mesh = ConvertOBJToMesh(data, options);
    mesh->SetFilePath(filepath);

    size_t nameStart = filepath.find_last_of("/\\");
    String filename = (nameStart != String::npos) ? filepath.substr(nameStart + 1) : filepath;
    size_t extPos = filename.find_last_of('.');
    if (extPos != String::npos) {
        filename = filename.substr(0, extPos);
    }
    mesh->SetName(filename);

    LOG_CORE_INFO("Loaded OBJ: {} ({} vertices, {} faces, {} normals, {} UVs)",
                  filepath,
                  data.Positions.size(),
                  data.Faces.size(),
                  data.Normals.size(),
                  data.TexCoords.size());

    return mesh;
}

Ref<Mesh> MeshLoader::CreateCube(float size) {
    float s = size * 0.5f;

    Vector<Vertex> vertices = {
        // Front
        {{-s, -s,  s}, { 0,  0,  1}, {0, 0}, {1, 0, 0}, {0, 1, 0}},
        {{ s, -s,  s}, { 0,  0,  1}, {1, 0}, {1, 0, 0}, {0, 1, 0}},
        {{ s,  s,  s}, { 0,  0,  1}, {1, 1}, {1, 0, 0}, {0, 1, 0}},
        {{-s,  s,  s}, { 0,  0,  1}, {0, 1}, {1, 0, 0}, {0, 1, 0}},
        // Back
        {{ s, -s, -s}, { 0,  0, -1}, {0, 0}, {-1, 0, 0}, {0, 1, 0}},
        {{-s, -s, -s}, { 0,  0, -1}, {1, 0}, {-1, 0, 0}, {0, 1, 0}},
        {{-s,  s, -s}, { 0,  0, -1}, {1, 1}, {-1, 0, 0}, {0, 1, 0}},
        {{ s,  s, -s}, { 0,  0, -1}, {0, 1}, {-1, 0, 0}, {0, 1, 0}},
        // Top
        {{-s,  s,  s}, { 0,  1,  0}, {0, 0}, {1, 0, 0}, {0, 0, -1}},
        {{ s,  s,  s}, { 0,  1,  0}, {1, 0}, {1, 0, 0}, {0, 0, -1}},
        {{ s,  s, -s}, { 0,  1,  0}, {1, 1}, {1, 0, 0}, {0, 0, -1}},
        {{-s,  s, -s}, { 0,  1,  0}, {0, 1}, {1, 0, 0}, {0, 0, -1}},
        // Bottom
        {{-s, -s, -s}, { 0, -1,  0}, {0, 0}, {1, 0, 0}, {0, 0, 1}},
        {{ s, -s, -s}, { 0, -1,  0}, {1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{ s, -s,  s}, { 0, -1,  0}, {1, 1}, {1, 0, 0}, {0, 0, 1}},
        {{-s, -s,  s}, { 0, -1,  0}, {0, 1}, {1, 0, 0}, {0, 0, 1}},
        // Right
        {{ s, -s,  s}, { 1,  0,  0}, {0, 0}, {0, 0, -1}, {0, 1, 0}},
        {{ s, -s, -s}, { 1,  0,  0}, {1, 0}, {0, 0, -1}, {0, 1, 0}},
        {{ s,  s, -s}, { 1,  0,  0}, {1, 1}, {0, 0, -1}, {0, 1, 0}},
        {{ s,  s,  s}, { 1,  0,  0}, {0, 1}, {0, 0, -1}, {0, 1, 0}},
        // Left
        {{-s, -s, -s}, {-1,  0,  0}, {0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{-s, -s,  s}, {-1,  0,  0}, {1, 0}, {0, 0, 1}, {0, 1, 0}},
        {{-s,  s,  s}, {-1,  0,  0}, {1, 1}, {0, 0, 1}, {0, 1, 0}},
        {{-s,  s, -s}, {-1,  0,  0}, {0, 1}, {0, 0, 1}, {0, 1, 0}},
    };

    Vector<u32> indices = {
        0,  1,  2,  2,  3,  0,   // Front
        4,  5,  6,  6,  7,  4,   // Back
        8,  9,  10, 10, 11, 8,   // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20   // Left
    };

    auto mesh = CreateRef<Mesh>(std::move(vertices), std::move(indices));
    mesh->SetName("Cube");
    return mesh;
}

Ref<Mesh> MeshLoader::CreateSphere(float radius, u32 segments, u32 rings) {
    Vector<Vertex> vertices;
    Vector<u32> indices;

    for (u32 ring = 0; ring <= rings; ++ring) {
        float phi = static_cast<float>(ring) / rings * glm::pi<float>();
        for (u32 seg = 0; seg <= segments; ++seg) {
            float theta = static_cast<float>(seg) / segments * 2.0f * glm::pi<float>();

            Vertex vertex{};
            vertex.Position.x = radius * std::sin(phi) * std::cos(theta);
            vertex.Position.y = radius * std::cos(phi);
            vertex.Position.z = radius * std::sin(phi) * std::sin(theta);

            vertex.Normal = glm::normalize(vertex.Position);

            vertex.TexCoords.x = static_cast<float>(seg) / segments;
            vertex.TexCoords.y = static_cast<float>(ring) / rings;

            vertex.Tangent = glm::vec3(-std::sin(theta), 0, std::cos(theta));
            vertex.Bitangent = glm::cross(vertex.Normal, vertex.Tangent);

            vertices.push_back(vertex);
        }
    }

    for (u32 ring = 0; ring < rings; ++ring) {
        for (u32 seg = 0; seg < segments; ++seg) {
            u32 current = ring * (segments + 1) + seg;
            u32 next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    auto mesh = CreateRef<Mesh>(std::move(vertices), std::move(indices));
    mesh->SetName("Sphere");
    return mesh;
}

Ref<Mesh> MeshLoader::CreatePlane(float width, float height, u32 subdivisions) {
    Vector<Vertex> vertices;
    Vector<u32> indices;

    float stepX = width / subdivisions;
    float stepZ = height / subdivisions;
    float startX = -width * 0.5f;
    float startZ = -height * 0.5f;

    for (u32 z = 0; z <= subdivisions; ++z) {
        for (u32 x = 0; x <= subdivisions; ++x) {
            Vertex vertex{};
            vertex.Position = {startX + x * stepX, 0.0f, startZ + z * stepZ};
            vertex.Normal = {0.0f, 1.0f, 0.0f};
            vertex.TexCoords = {static_cast<float>(x) / subdivisions, static_cast<float>(z) / subdivisions};
            vertex.Tangent = {1.0f, 0.0f, 0.0f};
            vertex.Bitangent = {0.0f, 0.0f, 1.0f};
            vertices.push_back(vertex);
        }
    }

    for (u32 z = 0; z < subdivisions; ++z) {
        for (u32 x = 0; x < subdivisions; ++x) {
            u32 topLeft = z * (subdivisions + 1) + x;
            u32 topRight = topLeft + 1;
            u32 bottomLeft = topLeft + subdivisions + 1;
            u32 bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    auto mesh = CreateRef<Mesh>(std::move(vertices), std::move(indices));
    mesh->SetName("Plane");
    return mesh;
}

Ref<Mesh> MeshLoader::CreateCylinder(float radius, float height, u32 segments) {
    Vector<Vertex> vertices;
    Vector<u32> indices;

    float halfHeight = height * 0.5f;

    // Side vertices
    for (u32 i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / segments * 2.0f * glm::pi<float>();
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        // Bottom
        Vertex bottom{};
        bottom.Position = {radius * cosT, -halfHeight, radius * sinT};
        bottom.Normal = {cosT, 0, sinT};
        bottom.TexCoords = {static_cast<float>(i) / segments, 0};
        bottom.Tangent = {-sinT, 0, cosT};
        bottom.Bitangent = {0, 1, 0};
        vertices.push_back(bottom);

        // Top
        Vertex top{};
        top.Position = {radius * cosT, halfHeight, radius * sinT};
        top.Normal = {cosT, 0, sinT};
        top.TexCoords = {static_cast<float>(i) / segments, 1};
        top.Tangent = {-sinT, 0, cosT};
        top.Bitangent = {0, 1, 0};
        vertices.push_back(top);
    }

    // Side indices
    for (u32 i = 0; i < segments; ++i) {
        u32 b0 = i * 2;
        u32 t0 = b0 + 1;
        u32 b1 = (i + 1) * 2;
        u32 t1 = b1 + 1;

        indices.push_back(b0);
        indices.push_back(b1);
        indices.push_back(t0);

        indices.push_back(t0);
        indices.push_back(b1);
        indices.push_back(t1);
    }

    // Top cap center
    u32 topCenter = static_cast<u32>(vertices.size());
    Vertex tcv{};
    tcv.Position = {0, halfHeight, 0};
    tcv.Normal = {0, 1, 0};
    tcv.TexCoords = {0.5f, 0.5f};
    tcv.Tangent = {1, 0, 0};
    tcv.Bitangent = {0, 0, 1};
    vertices.push_back(tcv);

    // Bottom cap center
    u32 bottomCenter = static_cast<u32>(vertices.size());
    Vertex bcv{};
    bcv.Position = {0, -halfHeight, 0};
    bcv.Normal = {0, -1, 0};
    bcv.TexCoords = {0.5f, 0.5f};
    bcv.Tangent = {1, 0, 0};
    bcv.Bitangent = {0, 0, -1};
    vertices.push_back(bcv);

    // Cap vertices
    u32 capStart = static_cast<u32>(vertices.size());
    for (u32 i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) / segments * 2.0f * glm::pi<float>();
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        // Top cap vertex
        Vertex tv{};
        tv.Position = {radius * cosT, halfHeight, radius * sinT};
        tv.Normal = {0, 1, 0};
        tv.TexCoords = {0.5f + 0.5f * cosT, 0.5f + 0.5f * sinT};
        tv.Tangent = {1, 0, 0};
        tv.Bitangent = {0, 0, 1};
        vertices.push_back(tv);

        // Bottom cap vertex
        Vertex bv{};
        bv.Position = {radius * cosT, -halfHeight, radius * sinT};
        bv.Normal = {0, -1, 0};
        bv.TexCoords = {0.5f + 0.5f * cosT, 0.5f - 0.5f * sinT};
        bv.Tangent = {1, 0, 0};
        bv.Bitangent = {0, 0, -1};
        vertices.push_back(bv);
    }

    // Cap indices
    for (u32 i = 0; i < segments; ++i) {
        u32 t0 = capStart + i * 2;
        u32 t1 = capStart + (i + 1) * 2;
        u32 b0 = t0 + 1;
        u32 b1 = t1 + 1;

        // Top cap
        indices.push_back(topCenter);
        indices.push_back(t0);
        indices.push_back(t1);

        // Bottom cap
        indices.push_back(bottomCenter);
        indices.push_back(b1);
        indices.push_back(b0);
    }

    auto mesh = CreateRef<Mesh>(std::move(vertices), std::move(indices));
    mesh->SetName("Cylinder");
    return mesh;
}

} // namespace Engine
