#pragma once

#include "core/Types.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"

#include <glm/glm.hpp>

namespace Engine {

struct RenderCommand {
    Ref<Shader> ShaderRef;
    Ref<VertexArray> VAORef;
    glm::mat4 Transform{1.0f};
    u32 IndexCount = 0;
    u32 SortKey = 0;

    RenderCommand() = default;

    RenderCommand(Ref<Shader> shader, Ref<VertexArray> vao,
                  const glm::mat4& transform, u32 indexCount)
        : ShaderRef(shader), VAORef(vao), Transform(transform), IndexCount(indexCount) {
        // Generate sort key from shader ID for efficient sorting
        SortKey = shader ? shader->GetRendererID() : 0;
    }

    bool operator<(const RenderCommand& other) const {
        return SortKey < other.SortKey;
    }
};

} // namespace Engine
