#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <vector>
#include "SimulationApp.hpp"
#include "UISettings.hpp"
#include "pso/PSO.hpp"
#include "pso/Objective.hpp"

// ============================================================
//  Vec3 + operators (file scope so PSOApp.cpp's free helpers see them)
// ============================================================
struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };

inline Vec3  v3(float x, float y, float z)       { return {x, y, z}; }
inline Vec3  operator+(const Vec3& a, const Vec3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3  operator-(const Vec3& a, const Vec3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3  operator*(const Vec3& a, float s)       { return {a.x*s,   a.y*s,   a.z*s}; }
inline float dot3  (const Vec3& a, const Vec3& b)    { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3  cross3(const Vec3& a, const Vec3& b) {
    return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
inline float len3 (const Vec3& a) { return std::sqrt(dot3(a, a)); }
inline Vec3  norm3(const Vec3& a) {
    const float l = len3(a);
    return l > 1e-6f ? a * (1.0f / l) : Vec3{0.0f, 0.0f, 0.0f};
}

// ============================================================
//  PSOApp
// ============================================================
class PSOApp : public SimulationApp {
public:
    enum class ViewMode { HEATMAP_2D, SURFACE_3D };

    PSOApp(int width, int height, UISettings* ui);

    const char* name() const override { return "PSO"; }

    void handleEvent(const sf::Event& e, sf::RenderWindow& w) override;
    void update(float dt) override;
    void render(sf::RenderWindow& w) override;
    void drawUI(sf::RenderWindow& w) override;
    void drawStatsHUD() override;
    void reset() override;

    bool ownsRunControls() const override { return true; }
    bool isRunning()       const override { return running_; }
    bool isFinished()      const override { return sim_.complete(); }
    void setRunning(bool r)      override { running_ = r; stepTimer_ = 0.0f; }
    void stepOnce()              override;

private:
    void drawAxes3D();
    void drawAxes2D();
    // ---------------- state ----------------
    int W_, H_;
    UISettings* ui_;
    pso::PSO        sim_;
    pso::PSOParams  params_;
    pso::ObjectiveId objective_ = pso::ObjectiveId::RASTRIGIN;
    ViewMode viewMode_ = ViewMode::SURFACE_3D;

    // ---------------- 2-D view ----------------
    struct View2D {
        float offsetX = 0.0f, offsetY = 0.0f;
        float scale   = 1.0f;
        sf::Vector2f toScreen(float x, float y) const {
            return { offsetX + x * scale, offsetY + y * scale };
        }
        sf::Vector2f toScreen(const Vector2& p) const { return toScreen(p.x, p.y); }
    } view2d_;

    sf::Image   heatmapImg_;
    sf::Texture heatmapTex_;
    sf::Sprite  heatmapSprite_;
    bool        heatmapDirty_ = true;

    // ---------------- 3-D view ----------------
    struct OrbitCamera {
        float yaw      = 0.65f;
        float pitch    = 0.55f;
        float distance = 22.0f;
        float fovDeg   = 45.0f;
        Vec3  target   { 0.0f, 0.0f, 0.0f };

        Vec3  eye() const;
        bool  project(const Vec3& w, float screenW, float screenH,
                      sf::Vector2f& out) const;
        float depth(const Vec3& w) const;
    } camera_;

    struct SurfaceQuad { Vec3 v[4]; sf::Color color; };
    std::vector<SurfaceQuad> surfaceQuads_;
    bool    surfaceDirty_ = true;
    int     surfaceRes_   = 40;
    float   surfaceZMin_  = 0.0f;
    float   surfaceZMax_  = 1.0f;
    Vec3    surfaceCenter_ { 0.0f, 0.0f, 0.0f };

    // ---------------- run state ----------------
    bool  running_      = false;
    float stepTimer_    = 0.0f;
    float stepInterval_ = 0.10f;

    // ---------------- display options ----------------
    bool showParticles_     = true;
    bool showTrails_        = true;
    bool showPBest_         = false;
    bool showGBest_         = true;
    bool showGround_        = true;
    bool showVerticalDrops_ = true;
    bool showVelocityVec_   = false;
    int  trailLength_       = 25;

    // ---------------- input ----------------
    bool           dragging_  = false;
    sf::Vector2i   lastMouse_;

    // ---------------- log ----------------
    std::vector<std::pair<int, float>> improvements_;

    // ---------------- helpers ----------------
    void rebuildHeatmap();
    void rebuildSurface();
    void updateView2D();
    void advanceOneIteration();
    void setObjective(pso::ObjectiveId id);
    void render2D(sf::RenderWindow& w);
    void render3D(sf::RenderWindow& w);

        // --- z-compression for 3-D display ---
    float zDisplayScale_ = 1.0f;   // computed in rebuildSurface
    float zMinRaw_       = 0.0f;   // raw min of sampled surface
    float zMaxRaw_       = 1.0f;   // raw max
    float xySpan_        = 1.0f;   // domain width for relative sizing
    bool  autoFitZ_      = true;   // recompute zDisplayScale_ on rebuild

    // --- display toggles ---
    bool showAxes_      = true;
    bool showContours_  = true;
    bool showLegend_    = true;
    bool invertDrag_    = false;   // user preference

    // helper (define inline in header)
    float worldToDisplayZ(float v) const {
        return (v - zMinRaw_) * zDisplayScale_;
    }
};