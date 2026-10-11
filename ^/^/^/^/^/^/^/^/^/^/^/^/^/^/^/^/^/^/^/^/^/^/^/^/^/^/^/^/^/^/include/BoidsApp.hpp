#pragma once
#include <SFML/Graphics.hpp>
#include "SimulationApp.hpp"
#include "UISettings.hpp"
#include "Boid.h"
#include <vector>

class BoidsApp : public SimulationApp {
public:
    BoidsApp(int width, int height, UISettings* ui);

    const char* name() const override { return "Boids"; }

    void handleEvent(const sf::Event& e, sf::RenderWindow& w) override;
    void update(float dt) override;
    void render(sf::RenderWindow& w) override;
    void drawUI(sf::RenderWindow& w) override;
    void drawStatsHUD() override;
    void reset() override;

private:
    int W_, H_;
    UISettings* ui_;

    std::vector<Boid> boids_;

    // Tunables
    int   noOfBoids_     = 40;
    float scale_         = 40.0f;   // world->screen multiplier
    float boidSize_      = 5.0f;
    bool  showTrails_    = true;

    // Fade rectangle for the trail effect.
    sf::RectangleShape fade_;

    void respawn(int count);
    void computeWorldBounds(const sf::RenderWindow& w,
                            float& minX, float& maxX,
                            float& minY, float& maxY) const;
};