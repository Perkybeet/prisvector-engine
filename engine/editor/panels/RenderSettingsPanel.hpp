#pragma once

#include "Panel.hpp"

namespace Editor {

class RenderSettingsPanel : public Panel {
public:
    RenderSettingsPanel() : Panel("Render Settings") {}

    void OnImGuiRender() override;
};

} // namespace Editor
