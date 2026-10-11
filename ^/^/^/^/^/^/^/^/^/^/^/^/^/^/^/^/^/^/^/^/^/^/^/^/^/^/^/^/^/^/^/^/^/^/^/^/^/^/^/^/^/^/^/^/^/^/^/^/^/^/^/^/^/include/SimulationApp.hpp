#pragma once
#include <SFML/Graphics.hpp>

class SimulationApp {
public:
    virtual ~SimulationApp() = default;

    virtual const char* name() const = 0;

    virtual void handleEvent(const sf::Event& e, sf::RenderWindow& w) = 0;
    virtual void update(float dt)      = 0;
    virtual void render(sf::RenderWindow& w) = 0;
    virtual void drawUI(sf::RenderWindow& w) = 0;
    virtual void drawStatsHUD()        = 0;
    virtual void reset()               = 0;

    virtual void drawMenuBarItems() {}

    // Optional run-state hooks. Default = use the global ui.paused clock.
    virtual bool ownsRunControls() const { return false; }
    virtual bool isRunning()       const { return false; }
    virtual bool isFinished()      const { return false; }
    virtual void setRunning(bool)        {}
    virtual void stepOnce()              {}
};