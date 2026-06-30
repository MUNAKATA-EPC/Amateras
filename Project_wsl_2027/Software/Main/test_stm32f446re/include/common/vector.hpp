#pragma once
#include <Arduino.h>

class vector
{
private:
    float _vector_x;
    float _vector_y;

public:
    vector()
    {
        _vector_x = 0.0f;
        _vector_y = 0.0f;
    }
    vector(float x0, float y0, float x1, float y1)
    {
        _vector_x = x1 - x0;
        _vector_y = y1 - y0;
    }
    vector(float deg, float length)
    {
        float rad = radians(deg);

        _vector_x = length * cosf(rad);
        _vector_y = length * sinf(rad);
    }

    vector operator+(const vector &other) const
    {
        return vector(0.0f, 0.0f, _vector_x + other.x(), _vector_y + other.y());
    }
    vector operator-(const vector &other) const
    {
        return vector(0.0f, 0.0f, _vector_x - other.x(), _vector_y - other.y());
    }
    vector operator*(const float &scale) const
    {
        return vector(0.0f, 0.0f, _vector_x * scale, _vector_y * scale);
    }
    vector operator/(const float &scale) const
    {
        return vector(0.0f, 0.0f, _vector_x / scale, _vector_y / scale);
    }

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