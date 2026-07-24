#pragma once

#include <Arduino.h>

class led
{
private:
    uint8_t _pin;

public:
    bool begin(uint8_t pin)
    {
        _pin = pin;
        pinMode(_pin, OUTPUT);

        return true;
    }

    void lightup()
    {
        digitalWrite(_pin, HIGH);
        return;
    }

    void lightdown()
    {
        digitalWrite(_pin, LOW);
        return;
    }
};

inline led red_led;
inline led yellow_led;
inline led green_led;