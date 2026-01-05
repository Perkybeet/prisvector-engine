#pragma once

#include "Panel.hpp"

namespace Editor {

class InspectorPanel : public Panel {
public:
    InspectorPanel() : Panel("Inspector") {}

    void OnImGuiRender() override;

private:
    void DrawTransformComponent();
    void DrawMaterialComponent();
    void DrawRenderableComponent();
    void DrawMeshComponent();
    void DrawDirectionalLightComponent();
    void DrawPointLightComponent();
    void DrawSpotLightComponent();
    void DrawAmbientLightComponent();
    void DrawAddComponentButton();

    template<typename T>
    bool DrawComponentHeader(const char* label, bool canRemove = true);
};

} // namespace Editor
