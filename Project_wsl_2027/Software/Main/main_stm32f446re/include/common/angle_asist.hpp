#pragma once

#include <Arduino.h>

int16_t deg_error(int16_t target_deg, int16_t cur_deg)
{
    int16_t error = target_deg - cur_deg;
    if (error > 180)
        error -= 360;
    else if (error < -180)
        error += 360;
    return error;
}

float deg_error_float(float target_deg, float current_deg)
{
    float error = fmodf(target_deg - current_deg, 360.0f);
    if (error > 180.0f)
        error -= 360.0f;
    else if (error < -180.0f)
        error += 360.0f;
    return error;
}

float rad_error(float target_rad, float cur_rad)
{
    float error = target_rad - cur_rad;
    if (error > PI)
        error -= 2 * PI;
    else if (error < -PI)
        error += 2 * PI;
    return error;
}

int16_t deg_wrap(int16_t deg)
{
    int16_t wrapped = deg % 360;
    if (wrapped < 0)
        wrapped += 360;
    return wrapped;
}

float deg_wrap_float(float deg)
{
    float wrapped = fmodf(deg, 360.0f);
    if (wrapped < 0.0f)
        wrapped += 360.0f;
    return wrapped;
}