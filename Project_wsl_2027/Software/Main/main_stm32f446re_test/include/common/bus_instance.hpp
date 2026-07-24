#pragma once

#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>
#include <SPI.h>

HardwareSerial mySerial1(PA10, PA9);
HardwareSerial mySerial3(PC5, PB10);
HardwareSerial mySerial5(PD2, PC12);
HardwareSerial mySerial4(PA1, PA0);
HardwareSerial mySerial2(PA3, PA2);
HardwareSerial mySerial6(PC7, PC6);

TwoWire Wire1(PB9, PB8);
TwoWire Wire3(PC9, PA8);
