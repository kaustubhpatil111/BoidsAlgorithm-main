#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>

#include "UISettings.hpp"
#include "SimulationApp.hpp"
#include "BoidsApp.hpp"
#include "pso/PSOApp.hpp"

static const int WORLD_W = 1600;
static const int WORLD_H =  900;

static std::string modeName(UISettings::Mode m) {
    return m == UISettings::Mode::BOIDS ? "Boids" : "PSO";
}

// ---------------------------------------------------------------- style

static void applyImGuiStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 8.0f;
    s.FrameRounding     = 5.0f;
    s.GrabRounding      = 5.0f;
    s.ScrollbarRounding = 8.0f;
    s.TabRounding       = 5.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.WindowPadding     = ImVec2(12, 12);
    s.FramePadding      = ImVec2(8, 5);
    s.ItemSpacing       = ImVec2(8, 6);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.IndentSpacing     = 18.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.09f, 0.09f, 0.12f, 0.96f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.11f, 0.11f, 0.15f, 0.85f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.09f, 0.09f, 0.12f, 0.98f);
    c[ImGuiCol_Border]           = ImVec4(0.24f, 0.26f, 0.34f, 0.65f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.16f, 0.17f, 0.22f, 1.00f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.28f, 0.32f, 0.42f, 1.00f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    c[ImGuiCol_MenuBarBg]        = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    c[ImGuiCol_Header]           = ImVec4(0.20f, 0.24f, 0.34f, 1.00f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.28f, 0.34f, 0.48f, 1.00f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.32f, 0.40f, 0.56f, 1.00f);
    c[ImGuiCol_Button]           = ImVec4(0.20f, 0.24f, 0.34f, 1.00f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.30f, 0.40f, 0.58f, 1.00f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.38f, 0.50f, 0.72f, 1.00f);
    c[ImGuiCol_Tab]              = ImVec4(0.14f, 0.16f, 0.22f, 1.00f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.28f, 0.34f, 0.48f, 1.00f);
    c[ImGuiCol_TabActive]        = ImVec4(0.26f, 0.32f, 0.46f, 1.00f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.42f, 0.60f, 0.90f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.78f, 1.00f, 1.00f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.55f, 0.75f, 1.00f, 1.00f);
    c[ImGuiCol_Separator]        = ImVec4(0.24f, 0.26f, 0.34f, 0.55f);
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.22f, 0.26f, 0.34f, 1.00f);
    c[ImGuiCol_Text]             = ImVec4(0.92f, 0.94f, 0.98f, 1.00f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.55f, 0.58f, 0.66f, 1.00f);
    c[ImGuiCol_PlotLines]        = ImVec4(0.70f, 0.85f, 1.00f, 1.00f);
}

// ---------------------------------------------------------------- helpers

static void toggleRun(SimulationApp* active, UISettings& ui) {
    if (active->ownsRunControls()) active->setRunning(!active->isRunning());
    else                           ui.paused = !ui.paused;
}

static bool activeIsRunning(SimulationApp* active, const UISettings& ui) {
    return active->ownsRunControls() ? active->isRunning() : !ui.paused;
}

// ---------------------------------------------------------------- main

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    sf::ContextSettings ctx;
    ctx.antialiasingLevel = 4;

    sf::RenderWindow window(sf::VideoMode(WORLD_W, WORLD_H),
                            "Boids + PSO Learning Lab",
                            sf::Style::Default, ctx);
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    ImGui::SFML::Init(window);
    applyImGuiStyle();

    UISettings ui;

    BoidsApp boids(WORLD_W, WORLD_H, &ui);
    PSOApp   pso  (WORLD_W, WORLD_H, &ui);
    SimulationApp* active = &boids;

    // Letterboxed logical world view.
    sf::View worldView;
    auto updateWorldView = [&]() {
        sf::Vector2u ws = window.getSize();
        const float winW = float(ws.x), winH = float(ws.y);
        const float targetAspect = float(WORLD_W) / float(WORLD_H);
        const float winAspect    = winW / std::max(1.0f, winH);
        float vpW, vpH;
        if (winAspect > targetAspect) { vpH = 1.0f; vpW = targetAspect / winAspect; }
        else                          { vpW = 1.0f; vpH = winAspect / targetAspect; }

        worldView.setViewport(sf::FloatRect((1.0f - vpW) * 0.5f,
                                            (1.0f - vpH) * 0.5f, vpW, vpH));
        worldView.setSize(float(WORLD_W), float(WORLD_H));
        worldView.setCenter(float(WORLD_W) * 0.5f, float(WORLD_H) * 0.5f);
    };
    updateWorldView();

    bool showUI   = true;
    bool showHelp = false;
    bool requestFullscreen = false;

    sf::Clock clock;
    sf::Event event;

    while (window.isOpen()) {

        if (requestFullscreen) {
            requestFullscreen = false;
            ui.fullscreen = !ui.fullscreen;

            if (ui.fullscreen) {
                sf::Vector2u ws = window.getSize();
                ui.windowedW = ws.x; ui.windowedH = ws.y;
                window.create(sf::VideoMode::getDesktopMode(),
                              "Boids + PSO Learning Lab",
                              sf::Style::Fullscreen, ctx);
            } else {
                window.create(sf::VideoMode(ui.windowedW, ui.windowedH),
                              "Boids + PSO Learning Lab",
                              sf::Style::Default, ctx);
            }
            window.setFramerateLimit(60);
            window.setKeyRepeatEnabled(false);

            ImGui::SFML::Shutdown();
            ImGui::SFML::Init(window);
            applyImGuiStyle();
            updateWorldView();
        }

        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::Resized) updateWorldView();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Tab)  showUI = !showUI;
                if (event.key.code == sf::Keyboard::F1)   showHelp = !showHelp;
                if (event.key.code == sf::Keyboard::F11)  requestFullscreen = true;
                if (event.key.code == sf::Keyboard::R)    active->reset();
                if (event.key.code == sf::Keyboard::Num1) ui.mode = UISettings::Mode::BOIDS;
                if (event.key.code == sf::Keyboard::Num2) ui.mode = UISettings::Mode::PSO;
                if (event.key.code == sf::Keyboard::Space) toggleRun(active, ui);
            }

            active->handleEvent(event, window);
        }

        active = (ui.mode == UISettings::Mode::BOIDS)
               ? static_cast<SimulationApp*>(&boids)
               : static_cast<SimulationApp*>(&pso);

        ImGui::SFML::Update(window, clock.restart());
        ImGui::GetIO().FontGlobalScale = ui.fontScale;

        const float rawDt = 1.0f / 60.0f;
        float dt = rawDt * ui.simSpeed;
        if (active->ownsRunControls()) {
            dt = rawDt;                         // PSO owns its own timer
        } else {
            if (ui.paused && !ui.stepOnce) dt = 0.0f;
            if (ui.stepOnce) { dt = rawDt; ui.stepOnce = false; }
        }

        const float scrW = float(window.getSize().x);
        const float scrH = float(window.getSize().y);

        // ---------------- Menu bar ----------------
        if (ui.showMenuBar && ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Reset Current Mode", "R")) active->reset();
                ImGui::Separator();
                if (ImGui::MenuItem(ui.fullscreen ? "Exit Fullscreen" : "Enter Fullscreen", "F11"))
                    requestFullscreen = true;
                if (ImGui::MenuItem("Quit")) window.close();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Mode")) {
                bool b = (ui.mode == UISettings::Mode::BOIDS);
                if (ImGui::MenuItem("Boids", "1", b))  ui.mode = UISettings::Mode::BOIDS;
                if (ImGui::MenuItem("PSO",   "2", !b)) ui.mode = UISettings::Mode::PSO;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Control Panel", "Tab", &showUI);
                ImGui::MenuItem("Show Bottom Bar",    nullptr, &ui.showBottomBar);
                ImGui::MenuItem("Show Stats Overlay", nullptr, &ui.showStatsOverlay);
                ImGui::MenuItem("Show Menu Bar",      nullptr, &ui.showMenuBar);
                ImGui::Separator();
                ImGui::MenuItem("Fullscreen", "F11", &ui.fullscreen);
                if (ImGui::IsItemEdited()) requestFullscreen = true;
                ImGui::Separator();
                if (ImGui::MenuItem("Reset UI position/size")) {
                    ui.panelPos  = ImVec2(10.0f, 34.0f);
                    ui.panelSize = ImVec2(420.0f, 800.0f);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Simulation")) {
                if (ImGui::MenuItem("Reset", "R")) active->reset();

                if (active->ownsRunControls()) {
                    const bool running = active->isRunning();
                    if (ImGui::MenuItem(running ? "Pause" : "Run", "Space",
                                        false, !active->isFinished()))
                        active->setRunning(!running);
                    const bool canStep = !running && !active->isFinished();
                    if (ImGui::MenuItem("Step", nullptr, false, canStep))
                        active->stepOnce();
                } else {
                    if (ImGui::MenuItem(ui.paused ? "Resume" : "Pause", "Space"))
                        ui.paused = !ui.paused;
                    if (ImGui::MenuItem("Step", nullptr, false, ui.paused))
                        ui.stepOnce = true;
                }
                ImGui::Separator();
                if (!active->ownsRunControls())
                    ImGui::SliderFloat("Sim Speed", &ui.simSpeed, 0.0f, 20.0f, "%.2fx");
                else
                    ImGui::TextDisabled("Speed is set in the app's Execution panel.");
                ImGui::EndMenu();
            }
            active->drawMenuBarItems();
            if (ImGui::BeginMenu("Help")) {
                ImGui::MenuItem("Toggle Help", "F1", &showHelp);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        const float bottomBarH = ui.showBottomBar ? 46.0f : 0.0f;

        // ---------------- Main control panel ----------------
        if (showUI) {
            ImGui::SetNextWindowPos(ui.panelPos,   ImGuiCond_Always);
            ImGui::SetNextWindowSize(ui.panelSize, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(ui.opacity);
            ImGui::Begin("Learning Lab", &showUI, ImGuiWindowFlags_NoCollapse);

            ui.panelPos  = ImGui::GetWindowPos();
            ui.panelSize = ImGui::GetWindowSize();

            // Mode selector
            {
                ImVec2 full = ImGui::GetContentRegionAvail();
                float btnW = (full.x - 8.0f) * 0.5f;
                bool boidsActive = (ui.mode == UISettings::Mode::BOIDS);

                if (boidsActive)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.50f, 0.85f, 1.0f));
                if (ImGui::Button("Boids  (1)", ImVec2(btnW, 32)))
                    ui.mode = UISettings::Mode::BOIDS;
                if (boidsActive) ImGui::PopStyleColor();

                ImGui::SameLine();

                if (!boidsActive)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.50f, 0.85f, 1.0f));
                if (ImGui::Button("PSO  (2)", ImVec2(btnW, 32)))
                    ui.mode = UISettings::Mode::PSO;
                if (!boidsActive) ImGui::PopStyleColor();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            active->drawUI(window);

            ImGui::End();
        }

        // ---------------- Stats HUD ----------------
        if (ui.showStatsOverlay) {
            ImGui::SetNextWindowPos(ImVec2(scrW - 300.0f, 40.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(290.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.75f);
            ImGui::Begin("##stats", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
            ImGui::TextColored(ui.accent, "Learning Lab");
            ImGui::Separator();
            ImGui::Text("Mode: %s", modeName(ui.mode).c_str());
            active->drawStatsHUD();
            ImGui::End();
        }

        // ---------------- Bottom toolbar ----------------
        if (ui.showBottomBar) {
            ImGui::SetNextWindowPos(ImVec2(0.0f, scrH - bottomBarH));
            ImGui::SetNextWindowSize(ImVec2(scrW, bottomBarH));
            ImGui::SetNextWindowBgAlpha(0.92f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::Begin("##bottombar", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoBringToFrontOnFocus);

            const bool ownsRun  = active->ownsRunControls();
            const bool running  = activeIsRunning(active, ui);
            const bool finished = ownsRun && active->isFinished();

            if (ownsRun) {
                if (finished) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.55f, 0.35f, 1.0f));
                    if (ImGui::Button(" Reset ", ImVec2(90, 0))) active->reset();
                    ImGui::PopStyleColor();
                } else if (running) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.42f, 0.20f, 1.0f));
                    if (ImGui::Button(" Pause  ", ImVec2(90, 0))) active->setRunning(false);
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.60f, 0.32f, 1.0f));
                    if (ImGui::Button("  Run   ", ImVec2(90, 0))) active->setRunning(true);
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(running || finished);
                if (ImGui::Button(" Step ")) active->stepOnce();
                ImGui::EndDisabled();
            } else {
                if (ui.paused) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.60f, 0.32f, 1.0f));
                    if (ImGui::Button("  Play  ", ImVec2(90, 0))) ui.paused = false;
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.42f, 0.20f, 1.0f));
                    if (ImGui::Button(" Pause  ", ImVec2(90, 0))) ui.paused = true;
                    ImGui::PopStyleColor();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(!ui.paused);
                if (ImGui::Button(" Step ")) ui.stepOnce = true;
                ImGui::EndDisabled();
            }

            ImGui::SameLine();
            if (ImGui::Button(" Reset ")) active->reset();

            ImGui::SameLine();
            ImGui::Text(" | ");
            ImGui::SameLine();
            if (!ownsRun) {
                ImGui::Text("Speed:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(180.0f);
                ImGui::SliderFloat("##speed", &ui.simSpeed, 0.0f, 20.0f, "%.2fx");
            } else {
                ImGui::TextDisabled("Speed: per-app");
            }

            ImGui::SameLine();
            ImGui::Text(" | ");
            ImGui::SameLine();
            if (ImGui::Button("Mode: Boids")) ui.mode = UISettings::Mode::BOIDS;
            ImGui::SameLine();
            if (ImGui::Button("Mode: PSO"))   ui.mode = UISettings::Mode::PSO;

            ImGui::SameLine();
            ImGui::Text(" | ");
            ImGui::SameLine();
            if (ImGui::Button(ui.fullscreen ? "Windowed (F11)" : "Fullscreen (F11)"))
                requestFullscreen = true;

            float rightX = ImGui::GetWindowWidth() - 130.0f;
            if (ImGui::GetCursorPosX() < rightX) ImGui::SameLine(rightX);
            ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f),
                               "%.0f FPS", ImGui::GetIO().Framerate);

            ImGui::End();
            ImGui::PopStyleVar(2);
        }

        active->update(dt);

        window.clear(sf::Color(12, 12, 18));
        window.setView(worldView);
        active->render(window);
        window.setView(window.getDefaultView());

        if (showHelp) {
            ImGui::SetNextWindowPos(ImVec2(scrW * 0.5f - 260.0f, scrH * 0.5f - 190.0f),
                                    ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(520.0f, 380.0f), ImGuiCond_Always);
            ImGui::Begin("Help (F1)", &showHelp, ImGuiWindowFlags_NoCollapse);
            ImGui::TextWrapped(
                "=== Keyboard Shortcuts ===\n\n"
                "  Space  - Run / Pause (context-aware)\n"
                "  Tab    - Toggle control panel\n"
                "  F1     - Toggle this help\n"
                "  F11    - Fullscreen on/off\n"
                "  R      - Reset current mode\n"
                "  1 / 2  - Switch to Boids / PSO\n\n"
                "=== Boids ===\n"
                "  Boid Count, Zoom, Boid Size, Trails — see panel.\n\n"
                "=== PSO ===\n"
                "  Run / Pause, Step iteration, Reset — see panel.\n"
                "  Right arrow — step one iteration.\n"
                "  Rastrigin heatmap shows the search space;\n"
                "  particles and the global best are overlaid.\n");
            ImGui::End();
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}