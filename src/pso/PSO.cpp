#include "pso/PSO.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace pso {

static std::mt19937 g_rng;

void PSO::seedRng(unsigned seed) {
    g_rng.seed(seed);
}

float PSO::uniform(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(g_rng);
}

void PSO::reset(const PSOParams& params, Objective objective) {
    params_    = params;
    objective_ = std::move(objective);
    seedRng(params_.seed);

    particles_.clear();
    particles_.resize(params_.numParticles);

    const float lo = params_.lowerBound;
    const float hi = params_.upperBound;

    globalBestValue_ = params_.maximize ? -1e30f : 1e30f;
    iteration_ = 0;
    bestHistory_.clear();
    particleHistory_.clear();

    for (auto& p : particles_) {
        p.position = { uniform(lo, hi), uniform(lo, hi) };
        p.velocity = { uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f) };
        p.bestValue = objective_(p.position);
        p.bestPosition = p.position;

        if (isBetter(p.bestValue, globalBestValue_)) {
    globalBestValue_    = p.bestValue;
    globalBestPosition_ = p.bestPosition;
}
    }

    statsHistory_.clear();
{
    IterationStats s;
    s.bestValue = globalBestValue_;
    float sum = 0, sumSq = 0;
    for (auto& p : particles_) { sum += p.bestValue; sumSq += p.bestValue * p.bestValue; }
    const float n = float(particles_.size());
    s.meanValue = sum / n;
    s.stdValue  = std::sqrt(std::max(0.0f, sumSq / n - s.meanValue * s.meanValue));
    statsHistory_.push_back(s);
}

    bestHistory_.push_back(globalBestValue_);
    particleHistory_.push_back([&]{ std::vector<Vector2> v; v.reserve(particles_.size());
                                    for (auto& p : particles_) v.push_back(p.position);
                                    return v; }());
}

void PSO::stepIteration() {
    if (complete()) return;

    std::uniform_real_distribution<float> d01(0.0f, 1.0f);
    const float lo = params_.lowerBound;
    const float hi = params_.upperBound;

    for (auto& p : particles_) {
        const float r1x = d01(g_rng), r1y = d01(g_rng);
        const float r2x = d01(g_rng), r2y = d01(g_rng);

        // v = w*v + c1*r1*(pbest - x) + c2*r2*(gbest - x)
        const Vector2 cog{
            params_.c1 * r1x * (p.bestPosition.x - p.position.x),
            params_.c1 * r1y * (p.bestPosition.y - p.position.y)
        };
        const Vector2 soc{
            params_.c2 * r2x * (globalBestPosition_.x - p.position.x),
            params_.c2 * r2y * (globalBestPosition_.y - p.position.y)
        };

        p.velocity = p.velocity * params_.w + cog + soc;

        p.position = p.position + p.velocity;

        // Clip to bounds (matches numpy np.clip in the Python version)
        p.position.x = std::max(lo, std::min(hi, p.position.x));
        p.position.y = std::max(lo, std::min(hi, p.position.y));

        const float value = objective_(p.position);

        if (isBetter(value, p.bestValue)) {
    p.bestValue    = value;
    p.bestPosition = p.position;
}
if (isBetter(value, globalBestValue_)) {
    globalBestValue_    = value;
    globalBestPosition_ = p.position;
}
    }

    bestHistory_.push_back(globalBestValue_);
    std::vector<Vector2> snapshot;
    snapshot.reserve(particles_.size());
    for (const auto& p : particles_) snapshot.push_back(p.position);
    particleHistory_.push_back(std::move(snapshot));

    iteration_++;
    IterationStats s;
s.bestValue = globalBestValue_;
float sum = 0, sumSq = 0;
for (auto& p : particles_) {
    const float v = objective_(p.position);
    sum   += v;
    sumSq += v * v;
}
const float n = float(particles_.size());
s.meanValue = sum / n;
s.stdValue  = std::sqrt(std::max(0.0f, sumSq / n - s.meanValue * s.meanValue));
statsHistory_.push_back(s);
}

} // namespace pso