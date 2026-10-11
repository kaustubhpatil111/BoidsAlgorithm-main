#include "Vector2.h"
#include <cmath>

Vector2::Vector2(float x, float y)
{
    this->x = x;
    this->y = y;
}

Vector2 Vector2::operator+(const Vector2& other) const
{
    return Vector2(x + other.x, y + other.y);
}

Vector2 Vector2::operator-(const Vector2& other) const
{
    return Vector2(x - other.x, y - other.y);
}

Vector2 Vector2::operator*(float scalar) const
{
    return Vector2(x * scalar, y * scalar);
}
Vector2 Vector2::operator/(float scalar)const{
    return Vector2(x/scalar,y/scalar);
}
float Vector2::magnitude() const
{
    return std::sqrt(x * x + y * y);
}

Vector2 Vector2::normalized() const
{
    float length = magnitude();

    if (length == 0.0f)
    {
        return Vector2(0.0f, 0.0f);
    }

    return Vector2(x / length, y / length);
}

float Vector2::distanceTo(const Vector2& other) const
{
    Vector2 difference = *this - other;

    return difference.magnitude();
}