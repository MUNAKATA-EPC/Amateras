#include "vector.hpp"

Vector::Vector()
{
    _vector_x = 0.0f;
    _vector_y = 0.0f;
}
Vector::Vector(float x0, float y0, float x1, float y1)
{
    _vector_x = x1 - x0;
    _vector_y = y1 - y0;
}
Vector::Vector(float deg, float length)
{
    float rad = radians(deg);

    _vector_x = length * cosf(rad);
    _vector_y = length * sinf(rad);
}

Vector Vector::operator+(const Vector &other) const
{
    return Vector(0.0f, 0.0f, _vector_x + other.x(), _vector_y + other.y());
}
Vector Vector::operator-(const Vector &other) const
{
    return Vector(0.0f, 0.0f, _vector_x - other.x(), _vector_y - other.y());
}
Vector Vector::operator*(const float &scale) const
{
    return Vector(0.0f, 0.0f, _vector_x * scale, _vector_y * scale);
}
Vector Vector::operator/(const float &scale) const
{
    return Vector(0.0f, 0.0f, _vector_x / scale, _vector_y / scale);
}