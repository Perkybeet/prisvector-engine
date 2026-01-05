#pragma once

#include "core/Types.hpp"
#include "renderer/RenderCommand.hpp"

#include <algorithm>

namespace Engine {

class RenderQueue {
public:
    RenderQueue() = default;
    ~RenderQueue() = default;

    void Submit(const RenderCommand& command) {
        m_Commands.push_back(command);
    }

    void Submit(Ref<Shader> shader, Ref<VertexArray> vao,
                const glm::mat4& transform, u32 indexCount) {
        m_Commands.emplace_back(shader, vao, transform, indexCount);
    }

    void Sort() {
        std::sort(m_Commands.begin(), m_Commands.end());
    }

    void Flush();

    void Clear() {
        m_Commands.clear();
        m_Stats = {};
    }

    struct Statistics {
        u32 DrawCalls = 0;
        u32 ShaderBinds = 0;
        u32 Triangles = 0;
    };

    const Statistics& GetStats() const { return m_Stats; }

    usize GetCommandCount() const { return m_Commands.size(); }

private:
    Vector<RenderCommand> m_Commands;
    Statistics m_Stats;
};

} // namespace Engine
