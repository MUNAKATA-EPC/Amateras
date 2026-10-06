#pragma once

#include <Arduino.h>

class button
{
private:
    uint8_t _pin;
    uint8_t _pinmode;

    bool _pressed = false;
    bool _oldpressed = false;
    bool _released = false;
    bool _last_raw_state = false;

    unsigned long _last_debounce_time = 0;
    unsigned long _press_start_time = 0;
    unsigned long _pushing_time = 0;

    const unsigned long _debounce_delay = 20;

public:
    bool begin(uint8_t pin, uint8_t pinmode)
    {
        _pin = pin;
        _pinmode = pinmode;

        pinMode(_pin, _pinmode);

        bool raw_now = (digitalRead(_pin) == HIGH);

        if (_pinmode == INPUT_PULLDOWN)
        {
            _last_raw_state = raw_now;
        }
        else if (_pinmode == INPUT_PULLUP)
        {
            _last_raw_state = !raw_now;
        }
        else
        {
            _last_raw_state = raw_now;
        }

        _pressed = _last_raw_state;
        _oldpressed = _pressed;

        return true;
    }

    void update()
    {
        bool current_raw = false;
        if (_pinmode == INPUT_PULLDOWN)
        {
            current_raw = (digitalRead(_pin) == HIGH);
        }
        else if (_pinmode == INPUT_PULLUP)
        {
            current_raw = (digitalRead(_pin) == LOW);
        }
        else
        {
            current_raw = (digitalRead(_pin) == HIGH);
        }

        if (current_raw != _last_raw_state)
        {
            _last_debounce_time = millis();
        }

        if ((millis() - _last_debounce_time) > _debounce_delay)
        {
            if (current_raw != _pressed)
            {
                _pressed = current_raw;
            }
        }

        _last_raw_state = current_raw;
        if (_pressed)
        {
            if (!_oldpressed)
            {
                _press_start_time = millis();
            }
            _pushing_time = millis() - _press_start_time;
        }
        else
        {
            _pushing_time = 0;
        }
        _released = (!_pressed && _oldpressed);
        _oldpressed = _pressed;
    }

    bool isPushing() const { return _pressed; }
    bool isReleased() const { return _released; }

    unsigned long pushingTime() const { return _pushing_time; }

    void pushingTimeReset()
    {
        _pushing_time = 0;
        _press_start_time = millis();
    }
};