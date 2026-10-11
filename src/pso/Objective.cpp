#include "pso/Objective.hpp"
#include <cmath>

namespace pso {

static constexpr float PI_F = 3.14159265358979323846f;

static const ObjectiveInfo g_info[] = {
    {   "Rastrigin",
        "Highly multimodal. Many local minima on a regular grid.\n"
        "Classic test of exploration. Global min 0 at (0, 0).",
        -5.12f, 5.12f, -5.12f, 5.12f, {0, 0}, 0.0f, false },

    {   "Sphere",
        "Unimodal, convex. Trivial for most optimizers.\n"
        "Use as a sanity check. Global min 0 at (0, 0).",
        -5.12f, 5.12f, -5.12f, 5.12f, {0, 0}, 0.0f, false },

    {   "Rosenbrock",
        "Narrow curved valley ('banana'). Easy to enter, hard to traverse.\n"
        "Global min 0 at (1, 1).",
        -2.048f, 2.048f, -2.048f, 2.048f, {1, 1}, 0.0f, false },

    {   "Ackley",
        "Multimodal with a nearly-flat outer region and a deep well.\n"
        "Global min 0 at (0, 0).",
        -5.0f, 5.0f, -5.0f, 5.0f, {0, 0}, 0.0f, false },

    {   "Griewank",
        "Multimodal with widely-distributed local minima.\n"
        "Global min 0 at (0, 0).",
        -10.0f, 10.0f, -10.0f, 10.0f, {0, 0}, 0.0f, false },

    // ---------- maximization example ----------
    {   "Inverted Sphere (max)",
        "10 - (x^2 + y^2).\n"
        "Global MAXIMUM = 10 at (0, 0).\n"
        "Demonstrates maximization mode.",
        -5.12f, 5.12f, -5.12f, 5.12f, {0, 0}, 10.0f, true },
};

const ObjectiveInfo& objectiveInfo(ObjectiveId id) {
    int i = static_cast<int>(id);
    if (i < 0 || i >= static_cast<int>(ObjectiveId::COUNT)) i = 0;
    return g_info[i];
}

float evaluate(ObjectiveId id, const Vector2& p) {
    const float x = p.x, y = p.y;
    switch (id) {
        case ObjectiveId::RASTRIGIN:
            return 20.0f
                 + (x*x - 10.0f * std::cos(2.0f * PI_F * x))
                 + (y*y - 10.0f * std::cos(2.0f * PI_F * y));

        case ObjectiveId::SPHERE:
            return x*x + y*y;

        case ObjectiveId::ROSENBROCK:
            return (1.0f - x) * (1.0f - x)
                 + 100.0f * (y - x*x) * (y - x*x);

        case ObjectiveId::ACKLEY: {
            const float a = 20.0f, b = 0.2f, c = 2.0f * PI_F;
            const float sumSq  = x*x + y*y;
            const float sumCos = std::cos(c*x) + std::cos(c*y);
            return -a * std::exp(-b * std::sqrt(0.5f * sumSq))
                   - std::exp(0.5f * sumCos)
                   + a + std::exp(1.0f);
        }

        case ObjectiveId::GRIEWANK: {
            const float s = x*x + y*y;
            const float p = std::cos(x / std::sqrt(1.0f))
                          * std::cos(y / std::sqrt(2.0f));
            return 1.0f + s / 4000.0f - p;
        }

        case ObjectiveId::INVERTED_SPHERE:
            return 10.0f - (x*x + y*y);

        default: return 0.0f;
    }
}

} // namespace pso