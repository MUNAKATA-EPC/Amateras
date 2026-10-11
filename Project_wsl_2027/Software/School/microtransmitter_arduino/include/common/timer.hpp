#pragma once
#include <Arduino.h>

class timer
{
private:
    unsigned long _reset_time = 0UL;
    unsigned long _stop_time = 0UL;
    unsigned long _now_time = 0UL;

    bool _is_stopped = false;
    bool _ever_reset = false;

public:
    void reset()
    {
        _reset_time = millis();
        _is_stopped = false;

        if (!_ever_reset)
        {
            _ever_reset = true;
        }
    }
    void stop()
    {
        if (!_is_stopped)
        {
            _is_stopped = true;
            _stop_time = millis();
        }
    }
    void start()
    {
        if (_is_stopped)
        {
            _is_stopped = false;
            _reset_time = millis() - _stop_time + _reset_time;
        }
    }

    unsigned long msTime()
    {
        if (!_is_stopped)
        {
            _now_time = millis() - _reset_time;
        }
        else
        {
            _now_time = _stop_time - _reset_time;
        }

        return _now_time;
    }

    bool isStopped() const { return _is_stopped; }
    bool everReset() const { return _ever_reset; }
};
