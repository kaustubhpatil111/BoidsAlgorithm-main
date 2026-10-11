#include "BoidsApp.hpp"
#include <random>
#include <cmath>

static constexpr float PI_F = 3.14159265358979323846f;

BoidsApp::BoidsApp(int width, int height, UISettings* ui)
: W_(width), H_(height), ui_(ui)
{
    fade_.setSize(sf::Vector2f(float(W_), float(H_)));
    fade_.setFillColor(sf::Color(0, 0, 0, 30));
    reset();
}

void BoidsApp::computeWorldBounds(const sf::RenderWindow& w,
                                  float& minX, float& maxX,
                                  float& minY, float& maxY) const
{
    const float actualW = float(w.getSize().x);
    const float actualH = float(w.getSize().y);
    const float halfW = actualW / (2.0f * scale_);
    const float halfH = actualH / (2.0f * scale_);
    minX = -halfW; maxX = halfW;
    minY = -halfH; maxY = halfH;
}

void BoidsApp::respawn(int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xd(-20.0f, 20.0f);
    std::uniform_real_distribution<float> yd(-11.0f, 11.0f);
    std::uniform_real_distribution<float> vd(-1.0f, 1.0f);

    boids_.clear();
    boids_.reserve(count);
    for (int i = 0; i < count; ++i) {
        boids_.emplace_back(
            Vector2(xd(gen), yd(gen)),
            Vector2(vd(gen), vd(gen)),
            Vector2(0.0f, 0.0f));
    }
}

void BoidsApp::reset() { respawn(noOfBoids_); }

void BoidsApp::handleEvent(const sf::Event&, sf::RenderWindow&) {
    // No per-mode key handlers yet. Space/R are handled by main.cpp.
}

void BoidsApp::update(float dt) {
    if (dt <= 0.0f) return;

    for (auto& b : boids_) b.acceleration = b.steeringFrom(boids_);

    // World bounds are only used for wrap-around; use the current window.
    // Since update() doesn't get the window, use the initial stored size.
    // (For resize-correct wrapping we recompute in render(); the wrap is
    //  cheap and one frame of latency is invisible.)
    const float halfW = float(W_) / (2.0f * scale_);
    const float halfH = float(H_) / (2.0f * scale_);

    for (auto& b : boids_) {
        b.update(dt);
        b.wrapAround(-halfW, halfW, -halfH, halfH);
    }
}

void BoidsApp::render(sf::RenderWindow& w) {
    const float actualW = float(w.getSize().x);
    const float actualH = float(w.getSize().y);

    // Keep world bounds in sync with the actual window size each frame.
    W_ = int(actualW);
    H_ = int(actualH);
    fade_.setSize(sf::Vector2f(actualW, actualH));

    if (showTrails_) w.draw(fade_);

    const float centerX = actualW * 0.5f;
    const float centerY = actualH * 0.5f;

    for (const Boid& b : boids_) {
        const float screenX = centerX + b.position.x * scale_;
        const float screenY = centerY - b.position.y * scale_;

        sf::CircleShape tri(boidSize_, 3);
        tri.setOrigin(boidSize_, boidSize_);
        tri.setPosition(screenX, screenY);

        const float angle  = std::atan2(-b.velocity.y, b.velocity.x);
        const float degree = angle * 180.0f / PI_F;
        tri.setRotation(degree + 90.0f);

        tri.setFillColor(sf::Color::White);
        w.draw(tri);
    }
}

void BoidsApp::drawStatsHUD() {
    ImGui::Text("Mode:   Boids");
    ImGui::Text("Boids:  %d", int(boids_.size()));
    ImGui::Text("Scale:  %.1f px/unit", scale_);
}

void BoidsApp::drawUI(sf::RenderWindow&) {
    if (ImGui::CollapsingHeader("Boids", ImGuiTreeNodeFlags_DefaultOpen)) {
        int n = noOfBoids_;
        if (ImGui::SliderInt("Boid Count", &n, 5, 500)) {
            noOfBoids_ = n;
            respawn(noOfBoids_);
        }
        ImGui::SliderFloat("Zoom (px/unit)", &scale_, 10.0f, 120.0f);
        ImGui::SliderFloat("Boid Size", &boidSize_, 2.0f, 12.0f);
        ImGui::Checkbox("Trails (fade)", &showTrails_);
        if (ImGui::Button("Respawn")) reset();
    }
}