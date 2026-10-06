#pragma once

#include <Arduino.h>
// action
#include "action/motor.hpp"
// common
#include "common/angle_asist.hpp"
#include "common/bus_instance.hpp"
#include "common/serial_packet.hpp"
// device
#include "device/bno.hpp"
#include "device/button.hpp"
#include "device/cyclic_timer.hpp"
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
#include "module/ps3.hpp"
#include "module/ui.hpp"

void defence()
{
    motor::process(pd_gyro, gyro.deg(), 0);

    motor::move(0, 500);
}