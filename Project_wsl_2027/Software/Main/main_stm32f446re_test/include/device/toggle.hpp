#pragma once

#include <Arduino.h>

class toggle
{
private:
    uint8_t _pin;
    uint8_t _pinmode;

    bool _turned_on = false;

    bool _stable_judge(bool cur)
    {
// #define TREND_TRUE // trueになりやすいトグル判定
#ifdef TREND_TRUE
        static bool last_state = false;
        static uint8_t count = 0;
        if (cur)
        {
            count = 0;
            last_state = true;

            return true;
        }

        count++;

        if (count < 5)
        {
            return true;
        }
        return false;
#endif
#define TREND_FALSE // falseになりやすいトグル判定
#ifdef TREND_FALSE
        static bool last_state = false;
        static uint8_t count = 0;
        if (!cur)
        {
            count = 0;
            last_state = false;

            return false;
        }

        count++;

        if (count < 5)
        {
            return false;
        }
        return true;
#endif
    }

public:
    bool begin(uint8_t pin, uint8_t pinmode)
    {
        _pin = pin;
        _pinmode = pinmode;

        pinMode(_pin, _pinmode);

        return true;
    }

    void update()
    {
        bool cur = false;
        if (_pinmode == INPUT_PULLDOWN)
        {
            cur = (digitalRead(_pin) == HIGH);
        }
        else if (_pinmode == INPUT_PULLUP)
        {
            cur = (digitalRead(_pin) == LOW);
        }
        else
        {
            cur = (digitalRead(_pin) == HIGH);
        }

        _turned_on = _stable_judge(cur);
    }

    bool isTurnedOn() { return _turned_on; }
};

inline toggle action_toggle;