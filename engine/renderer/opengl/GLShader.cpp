#include "GLShader.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>

namespace Engine {

static GLenum ShaderTypeFromString(const std::string& type) {
    if (type == "vertex") return GL_VERTEX_SHADER;
    if (type == "fragment" || type == "pixel") return GL_FRAGMENT_SHADER;
    if (type == "geometry") return GL_GEOMETRY_SHADER;
    if (type == "compute") return GL_COMPUTE_SHADER;

    LOG_CORE_ERROR("Unknown shader type: {}", type);
    return 0;
}

Shader::Shader(const String& filepath) {
    std::string source = ReadFile(filepath);
    auto shaderSources = PreProcess(source);
    Compile(shaderSources);

    // Extract name from filepath
    auto lastSlash = filepath.find_last_of("/\\");
    lastSlash = lastSlash == String::npos ? 0 : lastSlash + 1;
    auto lastDot = filepath.rfind('.');
    auto count = lastDot == String::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
    m_Name = filepath.substr(lastSlash, count);
}

Shader::Shader(const String& name, const String& vertexSrc, const String& fragmentSrc)
    : m_Name(name) {
    std::unordered_map<u32, std::string> sources;
    sources[GL_VERTEX_SHADER] = vertexSrc;
    sources[GL_FRAGMENT_SHADER] = fragmentSrc;
    Compile(sources);
}

Shader::~Shader() {
    glDeleteProgram(m_RendererID);
}

std::string Shader::ReadFile(const String& filepath) {
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);

    if (in) {
        in.seekg(0, std::ios::end);
        result.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&result[0], static_cast<std::streamsize>(result.size()));
        in.close();
    } else {
        LOG_CORE_ERROR("Could not open shader file: {}", filepath);
    }

    return result;
}

std::unordered_map<u32, std::string> Shader::PreProcess(const std::string& source) {
    std::unordered_map<u32, std::string> shaderSources;

    const char* typeToken = "#type";
    size_t typeTokenLength = strlen(typeToken);
    size_t pos = source.find(typeToken, 0);

    while (pos != std::string::npos) {
        size_t eol = source.find_first_of("\r\n", pos);
        size_t begin = pos + typeTokenLength + 1;
        std::string type = source.substr(begin, eol - begin);

        size_t nextLinePos = source.find_first_not_of("\r\n", eol);
        pos = source.find(typeToken, nextLinePos);

        shaderSources[ShaderTypeFromString(type)] =
            (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSources;
}

void Shader::Compile(const std::unordered_map<u32, std::string>& shaderSources) {
    GLuint program = glCreateProgram();
    std::vector<GLuint> shaderIDs;
    shaderIDs.reserve(shaderSources.size());

    for (auto& [type, source] : shaderSources) {
        GLuint shader = glCreateShader(type);

        const GLchar* sourceCStr = source.c_str();
        glShaderSource(shader, 1, &sourceCStr, nullptr);
        glCompileShader(shader);

        GLint isCompiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
        if (isCompiled == GL_FALSE) {
            GLint maxLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

            glDeleteShader(shader);

            LOG_CORE_ERROR("Shader compilation failed: {}", infoLog.data());
            return;
        }

        glAttachShader(program, shader);
        shaderIDs.push_back(shader);
    }

    glLinkProgram(program);

    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        GLint maxLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

        glDeleteProgram(program);

        for (auto id : shaderIDs) {
            glDeleteShader(id);
        }

        LOG_CORE_ERROR("Shader linking failed: {}", infoLog.data());
        return;
    }

    for (auto id : shaderIDs) {
        glDetachShader(program, id);
        glDeleteShader(id);
    }

    m_RendererID = program;
}

void Shader::Bind() const {
    glUseProgram(m_RendererID);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

i32 Shader::GetUniformLocation(const String& name) {
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) {
        return m_UniformLocationCache[name];
    }

    i32 location = glGetUniformLocation(m_RendererID, name.c_str());
    m_UniformLocationCache[name] = location;
    return location;
}

void Shader::SetInt(const String& name, i32 value) {
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetIntArray(const String& name, i32* values, u32 count) {
    glUniform1iv(GetUniformLocation(name), static_cast<GLsizei>(count), values);
}

void Shader::SetFloat(const String& name, f32 value) {
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetFloat2(const String& name, const glm::vec2& value) {
    glUniform2f(GetUniformLocation(name), value.x, value.y);
}

void Shader::SetFloat3(const String& name, const glm::vec3& value) {
    glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetFloat4(const String& name, const glm::vec4& value) {
    glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetMat3(const String& name, const glm::mat3& value) {
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetMat4(const String& name, const glm::mat4& value) {
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

} // namespace Engine
