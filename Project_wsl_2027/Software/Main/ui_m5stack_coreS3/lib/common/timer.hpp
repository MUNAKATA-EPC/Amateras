#pragma once
#include <Arduino.h>

class Timer
{
private:
    unsigned long _reset_time = 0UL;
    unsigned long _stop_time = 0UL;
    unsigned long _now_time = 0UL;

    bool _is_stopped = false;
    bool _ever_reset = false;

public:
    void reset();
    void stop();
    void start();

    unsigned long msTime();

    bool isStopped() const { return _is_stopped; }
    bool everReset() const { return _ever_reset; }
};
