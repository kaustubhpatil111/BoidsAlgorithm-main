#pragma once
#include "Vector2.h"

namespace pso {

// 2-D Rastrigin, matches the Python version exactly:
//   20 + (x^2 - 10*cos(2*pi*x)) + (y^2 - 10*cos(2*pi*y))
float rastrigin(const Vector2& p);

// Search-space bounds used by the assignment.
constexpr float RASTRIGIN_LOWER = -5.12f;
constexpr float RASTRIGIN_UPPER =  5.12f;

} // namespace pso