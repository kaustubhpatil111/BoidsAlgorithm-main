#pragma once
#include "Vector2.h"

namespace pso {

enum class ObjectiveId {
    RASTRIGIN = 0,
    SPHERE,
    ROSENBROCK,
    ACKLEY,
    GRIEWANK,
    INVERTED_SPHERE,   // <-- maximization demo
    COUNT
};

struct ObjectiveInfo {
    const char* name;
    const char* description;
    float lowerX, upperX;
    float lowerY, upperY;
    Vector2 optimumPosition;
    float   optimumValue;
    bool    maximize;          // true = this objective's optimum is a maximum
};

const ObjectiveInfo& objectiveInfo(ObjectiveId id);
float evaluate(ObjectiveId id, const Vector2& p);

} // namespace pso