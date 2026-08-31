#pragma once

#include <Arduino.h>
// common
#include "common/serial_packet.hpp"
#include "common/bus_instance.hpp"
// device
#include "device/bno.hpp"
#include "device/button.hpp"
#include "device/led.hpp"
#include "device/toggle.hpp"
// device
#include "device/bno.hpp"
#include "device/button.hpp"
#include "device/led.hpp"
#include "device/toggle.hpp"
// module
#include "module/camera.hpp"
#include "module/lidar.hpp"
#include "module/line.hpp"
#include "module/motordriver.hpp"
#include "module/ui.hpp"

void offence()
{
    int sign = (sub1_toggle.isTurnedOn()) ? 1 : -1;
    motordriver::move(sign * 500, sign * 500, sign * 500, sign * 500);
}