#include "RenderQueue.hpp"

#include <glad/gl.h>

namespace Engine {

void RenderQueue::Flush() {
    m_Stats = {};

    u32 lastShaderID = 0;

    for (const auto& cmd : m_Commands) {
        if (!cmd.ShaderRef || !cmd.VAORef) {
            continue;
        }

        // Only bind shader if different from last
        u32 shaderID = cmd.ShaderRef->GetRendererID();
        if (shaderID != lastShaderID) {
            cmd.ShaderRef->Bind();
            lastShaderID = shaderID;
            m_Stats.ShaderBinds++;
        }

        // Set transform uniform
        cmd.ShaderRef->SetMat4("u_Model", cmd.Transform);

        // Bind VAO and draw
        cmd.VAORef->Bind();
        glDrawElements(GL_TRIANGLES, cmd.IndexCount, GL_UNSIGNED_INT, nullptr);

        m_Stats.DrawCalls++;
        m_Stats.Triangles += cmd.IndexCount / 3;
    }
}

} // namespace Engine
