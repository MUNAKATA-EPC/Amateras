#include <Arduino.h>
#include "dribbler.hpp"

void setup()
{
    dribbler1.init(23);
}

void loop()
{
    dribbler1.move(1300);
    delay(10);
}