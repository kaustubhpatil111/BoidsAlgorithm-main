#include "pso/Rastrigin.hpp"
#include <cmath>

namespace pso {

static constexpr float PI = 3.14159265358979323846f;

float rastrigin(const Vector2& p) {
    return 20.0f
         + (p.x * p.x - 10.0f * std::cos(2.0f * PI * p.x))
         + (p.y * p.y - 10.0f * std::cos(2.0f * PI * p.y));
}

} // namespace pso