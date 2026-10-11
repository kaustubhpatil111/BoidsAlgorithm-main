#pragma once
#include "Vector2.h"
#include <functional>
#include <vector>

namespace pso {

struct IterationStats {
    float bestValue = 0.0f;
    float meanValue = 0.0f;
    float stdValue  = 0.0f;
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Vector2 bestPosition;
    float   bestValue = 1e30f;
};

struct PSOParams {
    int   numParticles  = 30;
    int   numIterations = 100;
    float w   = 0.7f;   // inertia
    float c1  = 1.5f;   // cognitive
    float c2  = 1.5f;   // social
    float lowerBound = -5.12f;
    float upperBound =  5.12f;
    unsigned seed = 42;
};

class PSO {
public:
    using Objective = std::function<float(const Vector2&)>;
    
    const std::vector<IterationStats>& statsHistory() const { return statsHistory_; }
    void reset(const PSOParams& params, Objective objective);

    // Run exactly one full PSO iteration (velocity update, position update,
    // clipping, evaluation, best updates, history append).
    void stepIteration();

    bool complete() const { return iteration_ >= params_.numIterations; }

    // ----- read-only access for the renderer / UI -----
    const PSOParams& params()          const { return params_; }
    const std::vector<Particle>& particles() const { return particles_; }
    const std::vector<float>&   bestHistory()    const { return bestHistory_; }
    const std::vector<std::vector<Vector2>>& particleHistory() const { return particleHistory_; }

    const Vector2& globalBestPosition() const { return globalBestPosition_; }
    float          globalBestValue()    const { return globalBestValue_; }
    int            iteration()          const { return iteration_; }

private:
    PSOParams params_;
    Objective objective_;
    
    std::vector<IterationStats> statsHistory_;
    
    std::vector<Particle> particles_;
    Vector2 globalBestPosition_{0.0f, 0.0f};
    float   globalBestValue_ = 1e30f;
    int     iteration_ = 0;

    std::vector<float> bestHistory_;
    std::vector<std::vector<Vector2>> particleHistory_;

    void seedRng(unsigned seed);
    float uniform(float lo, float hi);
};
} // namespace pso