#pragma once

#include "Vector2.h"
#include <vector>

class Boid
{
private:
    float maxAcceleration;
    float maxSpeed ;
    float minimumSpeed;
    float slowdownRadius ;
    float neighborRadius;

    Vector2 wanderDirection;

public:
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    

    Boid(
        const Vector2& position,
        const Vector2& velocity,
        const Vector2& acceleration
    );

    void update(float deltaTime);
    void seek(const Vector2& target);
    void wrapAround(float minX, float maxX, float minY, float maxY);

    Vector2 separationFrom(const Boid& other);
    Vector2 separation( const std::vector<Boid> & boids);
    Vector2 alignment(const std::vector<Boid>&boids);
    Vector2 cohesion(const std::vector<Boid>&boids);
    Vector2 wander();
    Vector2 steeringFrom(const std::vector<Boid>& boids);
};