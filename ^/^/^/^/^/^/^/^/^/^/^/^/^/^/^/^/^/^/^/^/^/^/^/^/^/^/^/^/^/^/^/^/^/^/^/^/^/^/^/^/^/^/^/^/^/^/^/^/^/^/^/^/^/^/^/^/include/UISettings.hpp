#pragma once
#include "imgui.h"

struct UISettings {
    enum class Mode { BOIDS, PSO };

    Mode   mode           = Mode::BOIDS;
    bool   paused         = false;
    bool   stepOnce       = false;
    float  simSpeed       = 1.0f;
    bool   fullscreen     = false;
    unsigned int windowedW = 1600;
    unsigned int windowedH =  900;
    float  opacity        = 0.95f;
    float  fontScale      = 1.0f;
    bool   showMenuBar    = true;
    bool   showBottomBar  = true;
    bool   showStatsOverlay = true;
    ImVec2 panelPos       = ImVec2(10.0f, 34.0f);
    ImVec2 panelSize      = ImVec2(420.0f, 800.0f);
    ImVec4 accent         = ImVec4(0.55f, 0.75f, 1.00f, 1.0f);
};