#pragma once

#include <Arduino.h>

int16_t degError(int16_t target_deg, int16_t cur_deg)
{
    int16_t error = target_deg - cur_deg;
    if (error > 180)
        error -= 360;
    else if (error < -180)
        error += 360;
    return error;
}

float radError(float target_rad, float cur_rad)
{
    float error = target_rad - cur_rad;
    if (error > PI)
        error -= 2 * PI;
    else if (error < -PI)
        error += 2 * PI;
    return error;
}