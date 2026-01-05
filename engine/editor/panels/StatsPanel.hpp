#pragma once

#include "Panel.hpp"

namespace Editor {

class StatsPanel : public Panel {
public:
    StatsPanel() : Panel("Statistics") {}

    void OnImGuiRender() override;
};

} // namespace Editor
