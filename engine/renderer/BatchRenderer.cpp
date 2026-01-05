#include "BatchRenderer.hpp"
#include "core/Logger.hpp"

#include <glad/gl.h>

namespace Engine {

BatchRenderer::BatchRenderer() {
    m_Transforms.reserve(MaxInstances);

    // Create instance buffer for mat4 transforms
    glGenBuffers(1, &m_InstanceBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceBufferID);
    glBufferData(GL_ARRAY_BUFFER, MaxInstances * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
}

BatchRenderer::~BatchRenderer() {
    glDeleteBuffers(1, &m_InstanceBufferID);
}

void BatchRenderer::Begin(Ref<Shader> shader, Ref<VertexArray> geometry) {
    if (m_InBatch) {
        LOG_CORE_WARN("BatchRenderer::Begin called while already in batch!");
        End();
    }

    m_CurrentShader = shader;
    m_CurrentGeometry = geometry;
    m_CurrentIndexCount = geometry->GetIndexBuffer()->GetCount();
    m_Transforms.clear();
    m_InBatch = true;
}

void BatchRenderer::Submit(const glm::mat4& transform) {
    if (!m_InBatch) {
        LOG_CORE_WARN("BatchRenderer::Submit called outside of Begin/End!");
        return;
    }

    if (m_Transforms.size() >= MaxInstances) {
        Flush();
    }

    m_Transforms.push_back(transform);
}

void BatchRenderer::End() {
    if (!m_InBatch) {
        return;
    }

    Flush();
    m_InBatch = false;
    m_CurrentShader = nullptr;
    m_CurrentGeometry = nullptr;
}

void BatchRenderer::Flush() {
    if (m_Transforms.empty()) {
        return;
    }

    // Upload transforms to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceBufferID);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    m_Transforms.size() * sizeof(glm::mat4),
                    m_Transforms.data());

    // Bind shader and geometry
    m_CurrentShader->Bind();
    m_CurrentGeometry->Bind();

    // Setup instance attributes (mat4 = 4 vec4s)
    // Assumes vertex attributes 0-1 are position/color, instance starts at 2
    const u32 instanceAttribStart = 2;
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceBufferID);

    for (u32 i = 0; i < 4; i++) {
        u32 attribIndex = instanceAttribStart + i;
        glEnableVertexAttribArray(attribIndex);
        glVertexAttribPointer(attribIndex, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4),
                              reinterpret_cast<void*>(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(attribIndex, 1);
    }

    // Draw instanced
    glDrawElementsInstanced(GL_TRIANGLES,
                            m_CurrentIndexCount,
                            GL_UNSIGNED_INT,
                            nullptr,
                            static_cast<GLsizei>(m_Transforms.size()));

    // Disable instance attributes
    for (u32 i = 0; i < 4; i++) {
        glDisableVertexAttribArray(instanceAttribStart + i);
    }

    m_Stats.DrawCalls++;
    m_Stats.InstanceCount += static_cast<u32>(m_Transforms.size());

    m_Transforms.clear();
}

} // namespace Engine
