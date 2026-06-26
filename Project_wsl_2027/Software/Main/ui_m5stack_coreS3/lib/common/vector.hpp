#pragma once
#include <Arduino.h>

class Vector
{
private:
    float _vector_x;
    float _vector_y;

public:
    Vector();
    Vector(float x0, float y0, float x1, float y1);
    Vector(float deg, float length);

    Vector operator+(const Vector &other) const;
    Vector operator-(const Vector &other) const;
    Vector operator*(const float &scale) const;
    Vector operator/(const float &scale) const;

    bool is_empty() const { return (_vector_x == 0.0f && _vector_y == 0.0f); }

    float x() const { return _vector_x; }
    float y() const { return _vector_y; }
    float length() const { return sqrtf(_vector_x * _vector_x + _vector_y * _vector_y); }

    float deg() const
    {
        float deg = degrees(atan2f(_vector_y, _vector_x));
        return deg;
    }

    float rad() const
    {
        float rad = atan2f(_vector_y, _vector_x);
        if (rad < 0.0f)
            rad += 2.0f * PI;
        return rad;
    }
};