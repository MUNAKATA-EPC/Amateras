#pragma once

#include <Arduino.h>

float normalizeDeg(float deg);

float diffDeg(float deg1, float deg2);

float nearSessenDeg(float target_deg, float enter_deg);

float areaIndexFromDeg(int n, float deg);
float degFromAreaIndex(int n, int index);

enum class Area4
{
    FRONT,
    RIGHT,
    BACK,
    LEFT
};
enum class Area8
{
    FRONT,
    FRONT_RIGHT,
    RIGHT,
    BACK_RIGHT,
    BACK,
    BACK_LEFT,
    LEFT,
    FRONT_LEFT
};
enum class Area16
{
    FRONT,
    FRONT_FRONT_RIGHT,
    FRONT_RIGHT,
    RIGHT_FRONT_RIGHT,
    RIGHT,
    RIGHT_BACK_RIGHT,
    BACK_RIGHT,
    BACK_BACK_RIGHT,
    BACK,
    BACK_BACK_LEFT,
    BACK_LEFT,
    LEFT_BACK_LEFT,
    LEFT,
    LEFT_FRONT_LEFT,
    FRONT_LEFT,
    FRONT_FRONT_LEFT
};
Area4 area4(int deg);
Area8 area8(int deg);
Area16 area16(int deg);
