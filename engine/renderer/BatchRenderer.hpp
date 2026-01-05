#pragma once

#include "core/Types.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "renderer/opengl/GLBuffer.hpp"

#include <glm/glm.hpp>

namespace Engine {

class BatchRenderer {
public:
    static constexpr u32 MaxInstances = 10000;

    BatchRenderer();
    ~BatchRenderer();

    void Begin(Ref<Shader> shader, Ref<VertexArray> geometry);
    void Submit(const glm::mat4& transform);
    void End();

    struct Statistics {
        u32 DrawCalls = 0;
        u32 InstanceCount = 0;
    };

    const Statistics& GetStats() const { return m_Stats; }
    void ResetStats() { m_Stats = {}; }

private:
    void Flush();

private:
    Ref<Shader> m_CurrentShader;
    Ref<VertexArray> m_CurrentGeometry;
    u32 m_CurrentIndexCount = 0;

    Vector<glm::mat4> m_Transforms;
    u32 m_InstanceBufferID = 0;

    Statistics m_Stats;
    bool m_InBatch = false;
};

} // namespace Engine
