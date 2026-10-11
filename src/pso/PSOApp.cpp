#include "pso/PSOApp.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

using namespace pso;

static constexpr float PI_F = 3.14159265358979323846f;

// -------- colour map (viridis-like) --------
static sf::Color mapColor(float t) {
    t = std::max(0.0f, std::min(1.0f, t));

    struct Stop { float t; float r, g, b; };
    static const Stop S[] = {
        { 0.00f,   0,   0, 140 },
        { 0.25f,   0, 180, 220 },
        { 0.50f,   0, 220,  80 },
        { 0.75f, 240, 220,  60 },
        { 1.00f, 200,  40,  40 },
    };
    for (int i = 0; i < 4; ++i) {
        if (t <= S[i+1].t) {
            const float k = (t - S[i].t) / (S[i+1].t - S[i].t);
            return sf::Color(
                sf::Uint8(S[i].r + k * (S[i+1].r - S[i].r)),
                sf::Uint8(S[i].g + k * (S[i+1].g - S[i].g)),
                sf::Uint8(S[i].b + k * (S[i+1].b - S[i].b)),
                255);
        }
    }
    return sf::Color(200, 40, 40, 255);
}

// ============================================================ OrbitCamera

Vec3 PSOApp::OrbitCamera::eye() const {
    const float cp = std::cos(pitch), sp = std::sin(pitch);
    const float cy = std::cos(yaw),   sy = std::sin(yaw);
    return target + v3(distance * cp * cy,
                       distance * cp * sy,
                       distance * sp);
}

bool PSOApp::OrbitCamera::project(const Vec3& world, float screenW, float screenH,
                                  sf::Vector2f& out) const
{
    const Vec3 e = eye();
    const Vec3 f = norm3(target - e);

    Vec3 worldUp = v3(0.0f, 0.0f, 1.0f);
    Vec3 s = cross3(f, worldUp);
    if (len3(s) < 0.001f) {
        worldUp = v3(1.0f, 0.0f, 0.0f);
        s = cross3(f, worldUp);
    }
    s = norm3(s);
    const Vec3 u = cross3(s, f);

    const Vec3 r = world - e;
    const float vx = dot3(r, s);
    const float vy = dot3(r, u);
    const float vz = dot3(r, f * -1.0f);

    if (vz < 0.05f) return false;

    const float fLen = 1.0f / std::tan(fovDeg * PI_F / 360.0f);
    const float ndcX = vx * fLen / vz;
    const float ndcY = vy * fLen / vz;

    const float sc = std::min(screenW, screenH) * 0.5f;
    out.x = screenW * 0.5f + ndcX * sc;
    out.y = screenH * 0.5f - ndcY * sc;
    return true;
}

float PSOApp::OrbitCamera::depth(const Vec3& w) const {
    const Vec3 e = eye();
    const Vec3 f = norm3(target - e);
    return dot3(w - e, f);
}

// ============================================================ ctor / reset

PSOApp::PSOApp(int width, int height, UISettings* ui)
: W_(width), H_(height), ui_(ui)
{
    setObjective(objective_);
    reset();
}

void PSOApp::setObjective(pso::ObjectiveId id) {
    objective_ = id;
    const auto& info = objectiveInfo(id);
    params_.lowerBound = info.lowerX;
    params_.upperBound = info.upperX;
    params_.maximize   = info.maximize;    // <-- CHANGED

    surfaceDirty_ = true;
    heatmapDirty_ = true;

    camera_.target   = v3(0.0f, 0.0f, 0.0f);
    camera_.distance = 22.0f;
    camera_.yaw      = 0.65f;
    camera_.pitch    = 0.55f;
}

void PSOApp::reset() {
    sim_.reset(params_, [this](const Vector2& p) { return evaluate(objective_, p); });
    running_    = false;
    stepTimer_  = 0.0f;
    heatmapDirty_ = true;
    surfaceDirty_ = true;
    improvements_.clear();
    improvements_.push_back({0, sim_.globalBestValue()});
}

// ============================================================ 2-D view / heatmap

void PSOApp::updateView2D() {
    const float left   = ui_->panelSize.x + 60.0f;
    const float top    = 60.0f;
    const float availW = float(W_) - left - 40.0f;
    const float availH = float(H_) - top  - 80.0f;

    const auto& info = objectiveInfo(objective_);
    const float worldW = info.upperX - info.lowerX;
    const float worldH = info.upperY - info.lowerY;
    const float margin = 40.0f;

    const float sx = (availW - 2.0f * margin) / std::max(1e-3f, worldW);
    const float sy = (availH - 2.0f * margin) / std::max(1e-3f, worldH);
    view2d_.scale = std::min(sx, sy);

    const float worldCx = 0.5f * (info.lowerX + info.upperX);
    const float worldCy = 0.5f * (info.lowerY + info.upperY);
    const float screenCx = left + availW * 0.5f;
    const float screenCy = top  + availH * 0.5f;

    view2d_.offsetX = screenCx - worldCx * view2d_.scale;
    view2d_.offsetY = screenCy - worldCy * view2d_.scale;
}

void PSOApp::rebuildHeatmap() {
    constexpr int N = 512;
    heatmapImg_.create(N, N);

    const auto& info = objectiveInfo(objective_);

    std::vector<float> samples(N * N);
    float vmin = 1e30f, vmax = -1e30f;
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            const float x = info.lowerX + (info.upperX - info.lowerX) * (float(i) / (N - 1));
            const float y = info.upperY - (info.upperY - info.lowerY) * (float(j) / (N - 1));
            const float v = evaluate(objective_, {x, y});
            samples[j * N + i] = v;
            vmin = std::min(vmin, v);
            vmax = std::max(vmax, v);
        }
    }
    const float inv   = (vmax > vmin) ? 1.0f / (vmax - vmin) : 0.0f;
    const float cStep = (vmax - vmin) / 12.0f;

    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            const float v = samples[j * N + i];
            const float t = (v - vmin) * inv;
            sf::Color c = mapColor(t);

            if (showContours_ && cStep > 1e-6f) {
                const float k = v / cStep;
                const float frac = k - std::floor(k);
                if (frac < 0.02f || frac > 0.98f) {
                    c.r = sf::Uint8(c.r * 0.35f);
                    c.g = sf::Uint8(c.g * 0.35f);
                    c.b = sf::Uint8(c.b * 0.35f);
                }
            }
            heatmapImg_.setPixel(unsigned(i), unsigned(j), c);
        }
    }
    zMinRaw_ = vmin;
    zMaxRaw_ = vmax;

    heatmapTex_.loadFromImage(heatmapImg_);
    heatmapTex_.setSmooth(true);
    heatmapSprite_.setTexture(heatmapTex_, true);

    const sf::Vector2f tl = view2d_.toScreen(info.lowerX, info.upperY);
    heatmapSprite_.setPosition(tl);
    heatmapSprite_.setScale(
        view2d_.scale * (info.upperX - info.lowerX) / float(N),
        view2d_.scale * (info.upperY - info.lowerY) / float(N));

    heatmapDirty_ = false;
}

// ============================================================ 3-D surface

void PSOApp::rebuildSurface() {
    surfaceQuads_.clear();

    const auto& info = objectiveInfo(objective_);
    const int N = surfaceRes_;
    if (N < 4) { surfaceDirty_ = false; return; }

    const float stepX = (info.upperX - info.lowerX) / float(N);
    const float stepY = (info.upperY - info.lowerY) / float(N);

    std::vector<std::vector<float>> rawZ(N + 1, std::vector<float>(N + 1));
    float zMin = 1e30f, zMax = -1e30f;
    for (int j = 0; j <= N; ++j) {
        for (int i = 0; i <= N; ++i) {
            const float x = info.lowerX + i * stepX;
            const float y = info.lowerY + j * stepY;
            const float z = evaluate(objective_, {x, y});
            rawZ[j][i] = z;
            zMin = std::min(zMin, z);
            zMax = std::max(zMax, z);
        }
    }
    zMinRaw_ = zMin;
    zMaxRaw_ = zMax;

    xySpan_ = std::max(info.upperX - info.lowerX, info.upperY - info.lowerY);
    const float targetZHeight = xySpan_ * 0.40f;
    const float zRange = std::max(1e-6f, zMax - zMin);
    if (autoFitZ_ || zDisplayScale_ <= 0.0f)
        zDisplayScale_ = targetZHeight / zRange;

    std::vector<std::vector<Vec3>> grid(N + 1, std::vector<Vec3>(N + 1));
    for (int j = 0; j <= N; ++j)
        for (int i = 0; i <= N; ++i) {
            const float x = info.lowerX + i * stepX;
            const float y = info.lowerY + j * stepY;
            const float zd = (rawZ[j][i] - zMin) * zDisplayScale_;
            grid[j][i] = v3(x, y, zd);
        }

    const float cx = 0.5f * (info.lowerX + info.upperX);
    const float cy = 0.5f * (info.lowerY + info.upperY);
    const float cz = 0.5f * (zMax - zMin) * zDisplayScale_;
    surfaceCenter_ = v3(cx, cy, cz);

    camera_.target   = v3(cx, cy, cz * 0.5f);
    camera_.distance = xySpan_ * 2.2f;

    const float zSpan = std::max(1e-3f, (zMax - zMin));
    surfaceQuads_.reserve(N * N);
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            SurfaceQuad q;
            q.v[0] = grid[j    ][i    ];
            q.v[1] = grid[j    ][i + 1];
            q.v[2] = grid[j + 1][i + 1];
            q.v[3] = grid[j + 1][i    ];
            const float zAvgRaw = 0.25f * (rawZ[j][i] + rawZ[j][i+1]
                                         + rawZ[j+1][i+1] + rawZ[j+1][i]);
            q.color = mapColor((zAvgRaw - zMin) / zSpan);
            surfaceQuads_.push_back(q);
        }
    }
    surfaceDirty_ = false;
}

// ============================================================ stepping

void PSOApp::advanceOneIteration() {
    if (sim_.complete()) return;
    const float prevBest = sim_.globalBestValue();
    sim_.stepIteration();
    const float newBest = sim_.globalBestValue();

    // <-- CHANGED: direction-aware improvement detection
    const bool improved = params_.maximize
                        ? (newBest > prevBest + 1e-6f)
                        : (newBest < prevBest - 1e-6f);

    if (improved)
        improvements_.push_back({sim_.iteration(), newBest});
    if (improvements_.size() > 200)
        improvements_.erase(improvements_.begin());
}

void PSOApp::stepOnce() { advanceOneIteration(); }

// ============================================================ events

void PSOApp::handleEvent(const sf::Event& e, sf::RenderWindow& w) {
    if (e.type == sf::Event::KeyPressed) {
        switch (e.key.code) {
            case sf::Keyboard::Right: stepOnce(); break;
            case sf::Keyboard::V:
                viewMode_ = (viewMode_ == ViewMode::HEATMAP_2D)
                          ? ViewMode::SURFACE_3D : ViewMode::HEATMAP_2D;
                break;
            default: break;
        }
    }

    if (viewMode_ != ViewMode::SURFACE_3D) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (e.type == sf::Event::MouseButtonPressed &&
        e.mouseButton.button == sf::Mouse::Left) {
        dragging_  = true;
        lastMouse_ = sf::Mouse::getPosition(w);
    }
    if (e.type == sf::Event::MouseButtonReleased &&
        e.mouseButton.button == sf::Mouse::Left) {
        dragging_ = false;
    }
    if (e.type == sf::Event::MouseMoved && dragging_) {
        const sf::Vector2i mp = sf::Mouse::getPosition(w);
        float dx = float(mp.x - lastMouse_.x);
        float dy = float(mp.y - lastMouse_.y);
        if (invertDrag_) { dx = -dx; dy = -dy; }

        camera_.yaw   -= dx * 0.006f;
        camera_.pitch -= dy * 0.006f;
        camera_.pitch = std::max(-1.50f, std::min(1.50f, camera_.pitch));
        lastMouse_ = mp;
    }
    if (e.type == sf::Event::MouseWheelScrolled) {
        camera_.distance *= (1.0f - e.mouseWheelScroll.delta * 0.10f);
        camera_.distance = std::max(0.5f, std::min(300.0f, camera_.distance));
    }
}

// ============================================================ update

void PSOApp::update(float dt) {
    if (dt <= 0.0f) return;
    if (!running_) return;
    if (sim_.complete()) { running_ = false; return; }

    stepTimer_ += dt;
    int safety = 0;
    while (running_ && stepTimer_ >= stepInterval_ && safety++ < 256) {
        stepTimer_ -= stepInterval_;
        advanceOneIteration();
        if (sim_.complete()) { running_ = false; break; }
    }
}

// ============================================================ render

void PSOApp::render(sf::RenderWindow& w) {
    sf::RectangleShape bg(sf::Vector2f{float(W_), float(H_)});
    bg.setFillColor(sf::Color(18, 18, 26, 255));
    w.draw(bg);

    if (viewMode_ == ViewMode::HEATMAP_2D) {
        render2D(w);
    } else {
        render3D(w);
    }
}

// ---------------------------------------------------------------- 2-D

void PSOApp::render2D(sf::RenderWindow& w) {
    updateView2D();
    if (heatmapDirty_) rebuildHeatmap();
    w.draw(heatmapSprite_);

    if (showTrails_ && !sim_.particleHistory().empty()) {
        const auto& hist = sim_.particleHistory();
        const int startIter = std::max(0, int(hist.size()) - trailLength_);
        for (size_t p = 0; p < sim_.particles().size(); ++p) {
            for (int i = startIter; i + 1 < int(hist.size()); ++i) {
                const sf::Vector2f a = view2d_.toScreen(hist[i][p]);
                const sf::Vector2f b = view2d_.toScreen(hist[i + 1][p]);
                sf::Vertex ln[2] = {
                    {a, sf::Color(255, 255, 255, 60)},
                    {b, sf::Color(255, 255, 255, 60)}
                };
                w.draw(ln, 2, sf::Lines);
            }
        }
    }

    if (showPBest_) {
        for (const auto& p : sim_.particles()) {
            sf::CircleShape dot(2.5f);
            dot.setOrigin(2.5f, 2.5f);
            dot.setPosition(view2d_.toScreen(p.bestPosition));
            dot.setFillColor(sf::Color(255, 220, 100, 180));
            w.draw(dot);
        }
    }

    if (showParticles_) {
        for (const auto& p : sim_.particles()) {
            sf::CircleShape c(5.0f);
            c.setOrigin(5.0f, 5.0f);
            c.setPosition(view2d_.toScreen(p.position));
            c.setFillColor(sf::Color(220, 245, 255, 240));
            c.setOutlineThickness(1.5f);
            c.setOutlineColor(sf::Color(20, 20, 30, 230));
            w.draw(c);
        }
    }

    if (showGBest_) {
        sf::CircleShape star(11.0f, 5);
        star.setOrigin(11.0f, 11.0f);
        star.setPosition(view2d_.toScreen(sim_.globalBestPosition()));
        star.setFillColor(sf::Color(255, 80, 80, 245));
        star.setOutlineThickness(2.5f);
        star.setOutlineColor(sf::Color(255, 250, 230, 255));
        w.draw(star);
    }

    if (showAxes_) drawAxes2D();
}

// ---------------------------------------------------------------- 3-D

void PSOApp::render3D(sf::RenderWindow& w) {
    if (surfaceDirty_) rebuildSurface();

    const auto& info = objectiveInfo(objective_);
    const float SW = float(W_), SH = float(H_);

    struct Sorted { float depth; int idx; };
    std::vector<Sorted> order;
    order.reserve(surfaceQuads_.size());
    for (int i = 0; i < int(surfaceQuads_.size()); ++i) {
        const auto& q = surfaceQuads_[i];
        const Vec3 c = (q.v[0] + q.v[1] + q.v[2] + q.v[3]) * 0.25f;
        order.push_back({ camera_.depth(c), i });
    }
    std::sort(order.begin(), order.end(),
              [](const Sorted& a, const Sorted& b){ return a.depth > b.depth; });

    sf::VertexArray mesh(sf::Quads, order.size() * 4);
    size_t vi = 0;
    for (const auto& s : order) {
        const auto& q = surfaceQuads_[s.idx];
        bool ok = true;
        sf::Vector2f p[4];
        for (int k = 0; k < 4; ++k)
            if (!camera_.project(q.v[k], SW, SH, p[k])) { ok = false; break; }
        const sf::Color col = ok ? q.color : sf::Color::Transparent;
        for (int k = 0; k < 4; ++k) {
            mesh[vi + k].position = ok ? p[k] : sf::Vector2f{0, 0};
            mesh[vi + k].color    = col;
        }
        vi += 4;
    }
    w.draw(mesh);

    if (showGround_) {
        const int gridN = 8;
        const float dx = (info.upperX - info.lowerX) / gridN;
        const float dy = (info.upperY - info.lowerY) / gridN;
        const sf::Color gc(90, 100, 130, 100);
        for (int i = 0; i <= gridN; ++i) {
            const float x = info.lowerX + i * dx;
            const float y = info.lowerY + i * dy;
            sf::Vector2f a, b;
            if (camera_.project(v3(x, info.lowerY, 0), SW, SH, a) &&
                camera_.project(v3(x, info.upperY, 0), SW, SH, b)) {
                sf::Vertex ln[2] = { {a, gc}, {b, gc} };
                w.draw(ln, 2, sf::Lines);
            }
            if (camera_.project(v3(info.lowerX, y, 0), SW, SH, a) &&
                camera_.project(v3(info.upperX, y, 0), SW, SH, b)) {
                sf::Vertex ln[2] = { {a, gc}, {b, gc} };
                w.draw(ln, 2, sf::Lines);
            }
        }
    }

    if (showTrails_ && !sim_.particleHistory().empty()) {
        const auto& hist = sim_.particleHistory();
        const int startIter = std::max(0, int(hist.size()) - trailLength_);
        const sf::Color tc(255, 255, 255, 55);
        for (size_t p = 0; p < sim_.particles().size(); ++p) {
            for (int i = startIter; i + 1 < int(hist.size()); ++i) {
                const Vector2& A = hist[i][p];
                const Vector2& B = hist[i + 1][p];
                const float za = worldToDisplayZ(evaluate(objective_, A));
                const float zb = worldToDisplayZ(evaluate(objective_, B));
                sf::Vector2f a, b;
                if (camera_.project(v3(A.x, A.y, za), SW, SH, a) &&
                    camera_.project(v3(B.x, B.y, zb), SW, SH, b)) {
                    sf::Vertex ln[2] = { {a, tc}, {b, tc} };
                    w.draw(ln, 2, sf::Lines);
                }
            }
        }
    }

    for (const auto& p : sim_.particles()) {
        const float zd = worldToDisplayZ(evaluate(objective_, p.position));
        sf::Vector2f s;
        if (!camera_.project(v3(p.position.x, p.position.y, zd), SW, SH, s))
            continue;

        if (showVerticalDrops_) {
            sf::Vector2f s0;
            if (camera_.project(v3(p.position.x, p.position.y, 0.0f), SW, SH, s0)) {
                sf::Vertex ln[2] = {
                    {s,  sf::Color(180, 230, 255, 90)},
                    {s0, sf::Color(180, 230, 255,  0)}
                };
                w.draw(ln, 2, sf::Lines);
            }
        }
        if (showParticles_) {
            sf::CircleShape c(5.0f);
            c.setOrigin(5.0f, 5.0f);
            c.setPosition(s);
            c.setFillColor(sf::Color(220, 245, 255, 240));
            c.setOutlineThickness(1.5f);
            c.setOutlineColor(sf::Color(20, 20, 30, 230));
            w.draw(c);
        }
        if (showPBest_) {
            const float zp = worldToDisplayZ(evaluate(objective_, p.bestPosition));
            sf::Vector2f sp;
            if (camera_.project(v3(p.bestPosition.x, p.bestPosition.y, zp), SW, SH, sp)) {
                sf::CircleShape dot(2.5f);
                dot.setOrigin(2.5f, 2.5f);
                dot.setPosition(sp);
                dot.setFillColor(sf::Color(255, 220, 100, 220));
                w.draw(dot);
            }
        }
    }

    if (showGBest_) {
        const Vector2& g = sim_.globalBestPosition();
        const float zg = worldToDisplayZ(evaluate(objective_, g));
        sf::Vector2f sg;
        if (camera_.project(v3(g.x, g.y, zg), SW, SH, sg)) {
            sf::CircleShape star(11.0f, 5);
            star.setOrigin(11.0f, 11.0f);
            star.setPosition(sg);
            star.setFillColor(sf::Color(255, 80, 80, 245));
            star.setOutlineThickness(2.5f);
            star.setOutlineColor(sf::Color(255, 250, 230, 255));
            w.draw(star);
        }
    }

    if (showAxes_) drawAxes3D();
}

void PSOApp::drawAxes3D() {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const auto& info = objectiveInfo(objective_);
    const float SW = float(W_), SH = float(H_);
    const ImU32 colLine = IM_COL32(240, 240, 240, 200);
    const ImU32 colTick = IM_COL32(220, 220, 220, 255);
    const ImU32 colText = IM_COL32(230, 235, 245, 255);

    auto label = [&](sf::Vector2f p, const char* txt, ImU32 c = IM_COL32(230,235,245,255)) {
        const ImVec2 sz = ImGui::CalcTextSize(txt);
        dl->AddText(ImVec2(p.x - sz.x * 0.5f, p.y + 6.0f), c, txt);
    };

    {
        sf::Vector2f a, b;
        if (camera_.project(v3(info.lowerX, info.lowerY, 0), SW, SH, a) &&
            camera_.project(v3(info.upperX, info.lowerY, 0), SW, SH, b)) {
            dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), colLine, 2.0f);
            const int ticks = 4;
            for (int i = 0; i <= ticks; ++i) {
                const float xv = info.lowerX + (info.upperX - info.lowerX) * i / ticks;
                sf::Vector2f tp;
                if (camera_.project(v3(xv, info.lowerY, 0), SW, SH, tp)) {
                    dl->AddCircleFilled(ImVec2(tp.x, tp.y), 3.0f, colTick);
                    char buf[32]; std::snprintf(buf, 32, "%.1f", xv);
                    label(tp, buf, colText);
                }
            }
            if (camera_.project(v3(info.upperX + 0.4f, info.lowerY, 0), SW, SH, b))
                dl->AddText(ImVec2(b.x + 6, b.y), IM_COL32(255,180,180,255), "x");
        }
    }

    {
        sf::Vector2f a, b;
        if (camera_.project(v3(info.lowerX, info.lowerY, 0), SW, SH, a) &&
            camera_.project(v3(info.lowerX, info.upperY, 0), SW, SH, b)) {
            dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), colLine, 2.0f);
            const int ticks = 4;
            for (int i = 0; i <= ticks; ++i) {
                const float yv = info.lowerY + (info.upperY - info.lowerY) * i / ticks;
                sf::Vector2f tp;
                if (camera_.project(v3(info.lowerX, yv, 0), SW, SH, tp)) {
                    dl->AddCircleFilled(ImVec2(tp.x, tp.y), 3.0f, colTick);
                    char buf[32]; std::snprintf(buf, 32, "%.1f", yv);
                    label(tp, buf, colText);
                }
            }
            if (camera_.project(v3(info.lowerX, info.upperY + 0.4f, 0), SW, SH, b))
                dl->AddText(ImVec2(b.x + 6, b.y), IM_COL32(180,255,180,255), "y");
        }
    }

    {
        const float zTopD = (zMaxRaw_ - zMinRaw_) * zDisplayScale_;
        sf::Vector2f a, b;
        if (camera_.project(v3(info.lowerX, info.lowerY, 0),    SW, SH, a) &&
            camera_.project(v3(info.lowerX, info.lowerY, zTopD), SW, SH, b)) {
            dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), colLine, 2.0f);
            const int ticks = 4;
            for (int i = 0; i <= ticks; ++i) {
                const float rawZ = zMinRaw_ + (zMaxRaw_ - zMinRaw_) * i / ticks;
                const float dispZ = worldToDisplayZ(rawZ);
                sf::Vector2f tp;
                if (camera_.project(v3(info.lowerX, info.lowerY, dispZ), SW, SH, tp)) {
                    dl->AddCircleFilled(ImVec2(tp.x, tp.y), 3.0f, colTick);
                    char buf[32]; std::snprintf(buf, 32, "%.0f", rawZ);
                    dl->AddText(ImVec2(tp.x + 6, tp.y - 7), colText, buf);
                }
            }
            if (camera_.project(v3(info.lowerX, info.lowerY, zTopD + 0.3f), SW, SH, b))
                dl->AddText(ImVec2(b.x + 6, b.y), IM_COL32(200,220,255,255), "f(x,y)");
        }
    }

    {
        const float zOptDisp = worldToDisplayZ(info.optimumValue);
        sf::Vector2f sp;
        if (camera_.project(v3(info.optimumPosition.x, info.optimumPosition.y, zOptDisp),
                            SW, SH, sp)) {
            dl->AddCircle(ImVec2(sp.x, sp.y), 12.0f, IM_COL32(255,255,120,255), 16, 2.5f);
            dl->AddText(ImVec2(sp.x + 14, sp.y - 8),
                        IM_COL32(255,255,120,255), "optimum");
        }
    }
}

void PSOApp::drawAxes2D() {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const auto& info = objectiveInfo(objective_);
    const ImU32 colText = IM_COL32(230, 235, 245, 255);
    const ImU32 colTick = IM_COL32(220, 220, 220, 255);
    const ImU32 colLine = IM_COL32(200, 210, 230, 160);

    {
        sf::Vector2f a = view2d_.toScreen(info.lowerX, info.lowerY);
        sf::Vector2f b = view2d_.toScreen(info.upperX, info.lowerY);
        dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), colLine, 1.5f);
        const int ticks = 4;
        for (int i = 0; i <= ticks; ++i) {
            const float xv = info.lowerX + (info.upperX - info.lowerX) * i / ticks;
            sf::Vector2f tp = view2d_.toScreen(xv, info.lowerY);
            dl->AddCircleFilled(ImVec2(tp.x, tp.y), 3.0f, colTick);
            char buf[32]; std::snprintf(buf, 32, "%.1f", xv);
            const ImVec2 sz = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(tp.x - sz.x * 0.5f, tp.y + 6), colText, buf);
        }
    }
    {
        sf::Vector2f a = view2d_.toScreen(info.lowerX, info.lowerY);
        sf::Vector2f b = view2d_.toScreen(info.lowerX, info.upperY);
        dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), colLine, 1.5f);
        const int ticks = 4;
        for (int i = 0; i <= ticks; ++i) {
            const float yv = info.lowerY + (info.upperY - info.lowerY) * i / ticks;
            sf::Vector2f tp = view2d_.toScreen(info.lowerX, yv);
            dl->AddCircleFilled(ImVec2(tp.x, tp.y), 3.0f, colTick);
            char buf[32]; std::snprintf(buf, 32, "%.1f", yv);
            const ImVec2 sz = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(tp.x - sz.x - 8, tp.y - sz.y * 0.5f), colText, buf);
        }
    }

    {
        sf::Vector2f sp = view2d_.toScreen(info.optimumPosition);
        dl->AddCircle(ImVec2(sp.x, sp.y), 10.0f, IM_COL32(255,255,120,255), 16, 2.5f);
        dl->AddText(ImVec2(sp.x + 12, sp.y - 8),
                    IM_COL32(255,255,120,255), "optimum");
    }

    if (showLegend_) {
        const float left   = ui_->panelSize.x + 60.0f;
        const float top    = 60.0f;
        const float availW = float(W_) - left - 40.0f;
        const float availH = float(H_) - top  - 80.0f;
        const float lx = left + availW - 40.0f;
        const float ly = top  + availH - 180.0f;
        const float lw = 22.0f;
        const float lh = 160.0f;

        const int steps = 64;
        for (int s = 0; s < steps; ++s) {
            const float t = 1.0f - float(s) / (steps - 1);
            const sf::Color c = mapColor(t);
            dl->AddRectFilled(ImVec2(lx, ly + s * lh / steps),
                              ImVec2(lx + lw, ly + (s + 1) * lh / steps),
                              IM_COL32(c.r, c.g, c.b, 255));
        }
        dl->AddRect(ImVec2(lx, ly), ImVec2(lx + lw, ly + lh),
                    IM_COL32(200, 200, 200, 200), 0, 0, 1.5f);

        char hi[32], lo[32];
        std::snprintf(hi, 32, "%.1f", zMaxRaw_);
        std::snprintf(lo, 32, "%.1f", zMinRaw_);
        dl->AddText(ImVec2(lx + lw + 4, ly - 4), IM_COL32(230,230,240,255), hi);
        dl->AddText(ImVec2(lx + lw + 4, ly + lh - 4), IM_COL32(230,230,240,255), lo);
        dl->AddText(ImVec2(lx + lw + 4, ly + lh * 0.5f - 8),
                    IM_COL32(230,230,240,255), "f(x,y)");
    }
}

// ============================================================ HUD

void PSOApp::drawStatsHUD() {
    const char* run = sim_.complete()
        ? "FINISHED"
        : (running_ ? "RUNNING" : "PAUSED");

    const auto& info = objectiveInfo(objective_);
    ImGui::Text("Mode:       PSO");
    ImGui::Text("Function:   %s", info.name);
    ImGui::Text("Goal:       %s", params_.maximize ? "MAXIMIZE" : "MINIMIZE");
    ImGui::Text("State:      %s", run);
    ImGui::Text("Iteration:  %d / %d",
                sim_.iteration(), params_.numIterations);
    ImGui::Text("Best value: %.5f", sim_.globalBestValue());

    if (params_.maximize == info.maximize) {
        ImGui::Text("Reference:  %.5f", info.optimumValue);
        ImGui::Text("Error:      %.5f",
                    std::fabs(sim_.globalBestValue() - info.optimumValue));
    } else {
        ImGui::TextDisabled("Reference:  n/a (manual override)");
    }

    ImGui::Text("Best (x,y): %.4f, %.4f",
                sim_.globalBestPosition().x,
                sim_.globalBestPosition().y);
}

// ============================================================ UI

void PSOApp::drawUI(sf::RenderWindow&) {

    // ============ Problem ============
    if (ImGui::CollapsingHeader("PSO - Problem", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& info = objectiveInfo(objective_);
        int id = static_cast<int>(objective_);
        const char* names[] = {
            "Rastrigin", "Sphere", "Rosenbrock",
            "Ackley", "Griewank", "Inverted Sphere (max)"
        };
        if (ImGui::Combo("Objective", &id, names, int(ObjectiveId::COUNT))) {
            setObjective(static_cast<ObjectiveId>(id));
            reset();
        }

        // <-- NEW: Min/Max radio buttons
        int mode = params_.maximize ? 1 : 0;
        if (ImGui::RadioButton("Minimize", &mode, 0)) {
            if (params_.maximize) { params_.maximize = false; reset(); }
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Maximize", &mode, 1)) {
            if (!params_.maximize) { params_.maximize = true; reset(); }
        }

        ImGui::TextWrapped("%s", info.description);
        ImGui::Separator();
        ImGui::Text("Domain:  [%.3f, %.3f]^2", info.lowerX, info.upperX);
        ImGui::Text("Optimum: (%.3f, %.3f)  f=%.5f",
                    info.optimumPosition.x, info.optimumPosition.y,
                    info.optimumValue);
        ImGui::Text("Objective's default mode: %s",
                    info.maximize ? "MAXIMIZE" : "MINIMIZE");
    }

    // ============ View ============
    if (ImGui::CollapsingHeader("PSO - View", ImGuiTreeNodeFlags_DefaultOpen)) {
        int vm = static_cast<int>(viewMode_);
        if (ImGui::RadioButton("2-D Heatmap", &vm, 0)) viewMode_ = ViewMode::HEATMAP_2D;
        ImGui::SameLine();
        if (ImGui::RadioButton("3-D Surface", &vm, 1)) {
            viewMode_ = ViewMode::SURFACE_3D;
            surfaceDirty_ = true;
        }

        if (viewMode_ == ViewMode::SURFACE_3D) {
            ImGui::SliderInt("Surface resolution", &surfaceRes_, 10, 80);
            if (ImGui::IsItemEdited()) surfaceDirty_ = true;

            ImGui::SliderFloat("Yaw",   &camera_.yaw,   -PI_F, PI_F, "%.2f rad");
            ImGui::SliderFloat("Pitch", &camera_.pitch, -1.50f, 1.50f, "%.2f rad");
            ImGui::SliderFloat("Distance", &camera_.distance, 0.5f, 200.0f);
            ImGui::SliderFloat("FOV",   &camera_.fovDeg, 20.0f, 100.0f, "%.0f deg");

            if (ImGui::Button("Reset / Fit Camera")) {
                surfaceDirty_ = true;
                autoFitZ_     = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("View from below")) {
                camera_.pitch    = -0.75f;
                camera_.yaw      = 0.65f;
                camera_.distance = 22.0f;
                surfaceDirty_    = true;
                autoFitZ_        = true;
            }
        }
    }

    // ============ Parameters ============
    if (ImGui::CollapsingHeader("PSO - Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= ImGui::SliderInt  ("Particles",  &params_.numParticles,  5, 200);
        changed |= ImGui::SliderInt  ("Iterations", &params_.numIterations, 1, 500);
        changed |= ImGui::SliderFloat("w (inertia)",&params_.w,   0.0f, 1.2f);
        changed |= ImGui::SliderFloat("c1 (cogn.)",&params_.c1,   0.0f, 4.0f);
        changed |= ImGui::SliderFloat("c2 (social)",&params_.c2,  0.0f, 4.0f);

        int s = int(params_.seed);
        if (ImGui::InputInt("Seed", &s)) { params_.seed = unsigned(std::max(0, s)); changed = true; }
        ImGui::SameLine();
        if (ImGui::Button("Randomize")) { params_.seed = unsigned(std::rand()); changed = true; }

        if (ImGui::Button("Apply & Reset")) changed = true;
        if (changed) {
            sim_.reset(params_, [this](const Vector2& p){ return evaluate(objective_, p); });
            running_ = false; stepTimer_ = 0.0f;
            heatmapDirty_ = true;
            improvements_.clear();
            improvements_.push_back({0, sim_.globalBestValue()});
        }
    }

    // ============ Execution ============
    if (ImGui::CollapsingHeader("PSO - Execution", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (sim_.complete()) {
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.55f, 1.0f), "FINISHED");
            if (ImGui::Button("Run Again")) reset();
        } else if (running_) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.42f, 0.20f, 1.0f));
            if (ImGui::Button("Pause")) running_ = false;
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.60f, 0.32f, 1.0f));
            if (ImGui::Button("Start")) { running_ = true; stepTimer_ = 0.0f; }
            ImGui::PopStyleColor();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(running_ || sim_.complete());
        if (ImGui::Button("Step")) advanceOneIteration();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Reset")) reset();

        ImGui::SliderFloat("Step interval", &stepInterval_, 0.02f, 1.0f, "%.2fs");
        ImGui::Text("Iteration %d / %d",
                    sim_.iteration(), params_.numIterations);
    }

    // ============ Visualization ============
    if (ImGui::CollapsingHeader("PSO - Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Particles",         &showParticles_);
        ImGui::Checkbox("Trails",            &showTrails_);
        ImGui::Checkbox("Personal bests",    &showPBest_);
        ImGui::Checkbox("Global best",       &showGBest_);
        ImGui::SliderInt("Trail length",     &trailLength_, 1, 100);
        ImGui::Checkbox("Axes & ticks",      &showAxes_);
        ImGui::Checkbox("Contour lines",     &showContours_);
        if (ImGui::IsItemEdited()) heatmapDirty_ = true;
        ImGui::Checkbox("Legend",            &showLegend_);

        if (viewMode_ == ViewMode::SURFACE_3D) {
            ImGui::Checkbox("Auto-fit Z axis",   &autoFitZ_);
            if (ImGui::IsItemEdited()) surfaceDirty_ = true;

            if (!autoFitZ_) {
                if (ImGui::SliderFloat("Z scale", &zDisplayScale_, 0.001f, 2.0f, "%.4f"))
                    surfaceDirty_ = true;
            }
            ImGui::Checkbox("Invert drag",       &invertDrag_);
        }
        if (viewMode_ == ViewMode::SURFACE_3D) {
            ImGui::Checkbox("Ground grid",       &showGround_);
            ImGui::Checkbox("Vertical drops",    &showVerticalDrops_);
            ImGui::Checkbox("Velocity vectors",  &showVelocityVec_);
        }
    }

    // ============ Convergence ============
    if (ImGui::CollapsingHeader("PSO - Convergence", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& best = sim_.bestHistory();
        const auto& stats = sim_.statsHistory();

        if (best.size() >= 2) {
            float lo = best[0], hi = best[0];
            for (float v : best) { lo = std::min(lo, v); hi = std::max(hi, v); }
            for (auto& s : stats) { lo = std::min(lo, s.meanValue); hi = std::max(hi, s.meanValue); }
            const float pad = (hi - lo) * 0.05f + 1e-3f;

            ImGui::Text("Global best");
            ImGui::PlotLines("##best", best.data(), int(best.size()),
                             0, nullptr, lo - pad, hi + pad,
                             ImVec2(-1.0f, 90.0f));

            if (!stats.empty()) {
                std::vector<float> means; means.reserve(stats.size());
                for (auto& s : stats) means.push_back(s.meanValue);
                ImGui::Text("Swarm mean");
                ImGui::PlotLines("##mean", means.data(), int(means.size()),
                                 0, nullptr, lo - pad, hi + pad,
                                 ImVec2(-1.0f, 70.0f));
            }
        } else {
            ImGui::TextDisabled("No history yet. Press Start or Step.");
        }
    }

    // ============ Swarm statistics ============
    if (ImGui::CollapsingHeader("PSO - Swarm statistics")) {
        if (!sim_.statsHistory().empty()) {
            const auto& s = sim_.statsHistory().back();
            ImGui::Text("Best value:  %.6f", s.bestValue);
            ImGui::Text("Mean fitness:%.6f", s.meanValue);
            ImGui::Text("Std fitness: %.6f", s.stdValue);

            const auto& best = sim_.bestHistory();
            if (best.size() >= 2) {
                const int k = std::min<int>(10, int(best.size()) - 1);
                const float delta = best[best.size() - 1 - k] - best.back();
                ImGui::Text("Improvement (last %d iter): %.6f", k, delta);
            }
        }

        if (ImGui::BeginTable("parts", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, 220.0f)))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("x");
            ImGui::TableSetupColumn("y");
            ImGui::TableSetupColumn("f(x)");
            ImGui::TableSetupColumn("|v|");
            ImGui::TableSetupColumn("d(gbest)");
            ImGui::TableHeadersRow();

            const Vector2& g = sim_.globalBestPosition();
            int row = 0;
            for (const auto& p : sim_.particles()) {
                const float f = evaluate(objective_, p.position);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", row++);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", p.position.x);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.4f", p.position.y);
                ImGui::TableSetColumnIndex(3); ImGui::Text("%.5f", f);
                ImGui::TableSetColumnIndex(4); ImGui::Text("%.4f", p.velocity.magnitude());
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.4f", std::sqrt((p.position.x - g.x) * (p.position.x - g.x)
                                            + (p.position.y - g.y) * (p.position.y - g.y)));
            }
            ImGui::EndTable();
        }
    }

    // ============ Improvements log ============
    if (ImGui::CollapsingHeader("PSO - Best-improvement log")) {
        if (improvements_.empty()) {
            ImGui::TextDisabled("No improvements logged yet.");
        } else if (ImGui::BeginTable("improvements", 2,
                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                     ImGuiTableFlags_ScrollY,
                     ImVec2(0.0f, 180.0f)))
        {
            ImGui::TableSetupColumn("Iteration");
            ImGui::TableSetupColumn("Best value");
            ImGui::TableHeadersRow();
            for (auto it = improvements_.rbegin(); it != improvements_.rend(); ++it) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", it->first);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.6f", it->second);
            }
            ImGui::EndTable();
        }
    }

    // ============ Results ============
    if (ImGui::CollapsingHeader("PSO - Results")) {
        const auto& info = objectiveInfo(objective_);
        ImGui::Text("Best x1     = %.6f", sim_.globalBestPosition().x);
        ImGui::Text("Best x2     = %.6f", sim_.globalBestPosition().y);
        ImGui::Text("%s     = %.6f",
                    params_.maximize ? "Maximum" : "Minimum",
                    sim_.globalBestValue());
        ImGui::Separator();
        if (params_.maximize == info.maximize) {
            ImGui::Text("Theoretical optimum:");
            ImGui::Text("  (%.3f, %.3f), f = %.5f",
                        info.optimumPosition.x, info.optimumPosition.y,
                        info.optimumValue);
            ImGui::Text("Absolute error: %.6f",
                        std::fabs(sim_.globalBestValue() - info.optimumValue));
        } else {
            ImGui::TextDisabled("Error: n/a when mode overrides the objective default.");
        }
    }
}