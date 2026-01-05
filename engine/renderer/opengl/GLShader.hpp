#pragma once

#include "core/Types.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

namespace Engine {

class Shader {
public:
    Shader(const String& filepath);
    Shader(const String& name, const String& vertexSrc, const String& fragmentSrc);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    void SetInt(const String& name, i32 value);
    void SetIntArray(const String& name, i32* values, u32 count);
    void SetFloat(const String& name, f32 value);
    void SetFloat2(const String& name, const glm::vec2& value);
    void SetFloat3(const String& name, const glm::vec3& value);
    void SetFloat4(const String& name, const glm::vec4& value);
    void SetMat3(const String& name, const glm::mat3& value);
    void SetMat4(const String& name, const glm::mat4& value);

    const String& GetName() const { return m_Name; }
    u32 GetRendererID() const { return m_RendererID; }

private:
    std::string ReadFile(const String& filepath);
    std::unordered_map<u32, std::string> PreProcess(const std::string& source);
    void Compile(const std::unordered_map<u32, std::string>& shaderSources);

    i32 GetUniformLocation(const String& name);

private:
    u32 m_RendererID = 0;
    String m_Name;
    mutable std::unordered_map<String, i32> m_UniformLocationCache;
};

} // namespace Engine
