#pragma once

#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>
#include <SPI.h>

inline HardwareSerial mySerial1(PA10, PA9); // pc
inline HardwareSerial mySerial3(PC5, PB10); // ui
inline HardwareSerial mySerial5(PD2, PC12); // m5
inline HardwareSerial mySerial4(PA1, PA0);  // line
inline HardwareSerial mySerial2(PA3, PA2);  // camera
inline HardwareSerial mySerial6(PC7, PC6);  // lidar

TwoWire Wire1(PB9, PB8);
TwoWire Wire3(PC9, PA8);