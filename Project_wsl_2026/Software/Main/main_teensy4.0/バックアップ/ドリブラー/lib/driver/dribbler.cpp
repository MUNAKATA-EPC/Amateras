#include "dribbler.hpp"

bool Dribbler::init(uint8_t pin, int min, int max)
{
    _pin = pin;
    _min = min;
    _max = max;

    _esc.writeMicroseconds(_max);

    if (_esc.attach(_pin, _min, _max))
    {
        /*_esc.writeMicroseconds(_max);
        delay(4000);*/
        _esc.writeMicroseconds(_min);
        delay(4000);

        return true;
    }

    return false;
}

void Dribbler::move(int power)
{
    int safe_power = constrain(power, _min, _max);
    _esc.writeMicroseconds(safe_power);
}

Dribbler dribbler1;