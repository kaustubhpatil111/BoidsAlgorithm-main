#pragma once

class Vector2
{
public:
    float x;
    float y;

    Vector2() : x(0.0f), y(0.0f) {}          // <-- NEW: default ctor
    Vector2(float x, float y);

    Vector2 operator+(const Vector2& other) const;
    Vector2 operator-(const Vector2& other) const;
    Vector2 operator*(float scalar) const;
    Vector2 operator/(float scalar) const;
    float magnitude() const;
    Vector2 normalized() const;
    float distanceTo(const Vector2& other) const;

    // Convenience operators for PSO (added earlier)
    Vector2& operator+=(const Vector2& other) { x += other.x; y += other.y; return *this; }
    Vector2& operator-=(const Vector2& other) { x -= other.x; y -= other.y; return *this; }
    float dot(const Vector2& o) const { return x * o.x + y * o.y; }
};