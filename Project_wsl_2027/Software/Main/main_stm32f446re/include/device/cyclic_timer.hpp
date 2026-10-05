#pragma once

#include <Arduino.h>

class cyclic_timer
{
private:
    HardwareTimer *_tim;
    volatile bool _called = false;

    void _callback()
    {
        _called = true;
    }

public:
    void begin(auto tim_type, uint32_t microseconds)
    {
        _tim = new HardwareTimer(tim_type);
        _tim->setOverflow(microseconds, MICROSEC_FORMAT);
        _tim->attachInterrupt(std::bind(&cyclic_timer::_callback, this));
        _tim->resume();
    }

    bool called()
    {
        if (_called)
        {
            _called = false;
            return true;
        }
        return false;
    }
};

inline cyclic_timer cyclic_timer_1ms;
inline cyclic_timer cyclic_timer_10ms;
inline cyclic_timer cyclic_timer_200ms;