#pragma once

#include <Arduino.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <utility/imumaths.h>
#include <SPI.h>

static Adafruit_BNO055 *_bno = nullptr;

static float _deg_normal = 0;
static float _deg_reset = 0;
static float _deg = 0;

bool bnoInit(TwoWire *wire, uint8_t address)
{
    if (_bno != nullptr)
    {
        delete _bno;
        _bno = nullptr;
    }
    _bno = new Adafruit_BNO055(BNO055_ID, address, wire);

    uint16_t last_time = millis();
    bool success = false;
    while ((millis() - last_time) < 1000UL)
    {
        success = _bno->begin(OPERATION_MODE_IMUPLUS);
        if (success)
        {
            break;
        }
        else
        {
            delay(50);
        }
    }

    if (success)
    {
        _bno->setExtCrystalUse(true);
    }
    return success;
}
void bnoUpdate(bool resetbtn){
    if (_bno == nullptr)
        return;

    imu::Vector<3> euler = _bno->getVector(Adafruit_BNO055::VECTOR_EULER);
    float current_raw_x = (float)euler.x();
    _deg_normal = current_raw_x;

    if (resetbtn)
    {
        _deg_reset = _deg_normal;
    }

    _deg = fmodf(_deg_normal - _deg_reset + 360.0f, 360.0f);
    if (_deg > 180.0f)
        _deg -= 360.0f;
}

float bnoDeg() { return _deg; }