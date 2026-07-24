#ifndef DRIBBLER_HPP
#define DRIBBLER_HPP

#include <Arduino.h>
#include <Servo.h>

class Dribbler {
private:
    Servo _esc;
    uint8_t _pin;
    int _min;
    int _max;

public:
    bool init(uint8_t pin, int min = 1000, int max = 2000);
    void move(int power);
};

extern Dribbler dribbler1;

#endif