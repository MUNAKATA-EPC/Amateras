#pragma once

#include <Arduino.h>
// action
#include "action/motor.hpp"
// common
#include "common/angle_asist.hpp"
#include "common/bus_instance.hpp"
#include "common/const_number.hpp"
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/vector.hpp"
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

bool flag = false;
vector cur = vector(0, 0);         // [mm]
static vector prev = vector(0, 0); // [mm]
vector velocity;                   // [mm/s]

void defence()
{
    if (cyclic_timer_10ms.called())
    {
        if (line::angel)
        {
            if (!flag)
            {
                cur = vector(static_cast<float>(line::angel_deg), static_cast<float>(line::angel_dis));
                velocity = vector(0, 0);
                prev = cur;
            }
            else
            {
                cur = vector(static_cast<float>(line::angel_deg), static_cast<float>(line::angel_dis));
                velocity = (cur - prev) * 100;
                prev = cur;
            }

            flag = true;
        }
        else
        {
            flag = false;
        }
    }

    motor::process(pd_gyro, gyro.deg(), 0);

    if (line::angel)
    {
        const float kp = 800.0f / LINEANGEL_RADIOUS, kd = 0.0f;
        vector kp_t = cur * kp;
        vector kd_t = velocity * (-kd);
        vector t = kp_t + kd_t;

        vector m;
        if (ps3::stick_ldis > 5)
        {
            vector a = vector(ps3::stick_ldeg, ps3::stick_ldis * 600 / 128);

            m = a + t;                                              // 合力
            vector t_a = a * ((a * t) / (a.length() * a.length())); // tのa方向成分
            if ((a + t_a).length() > a.length())
            {
                m = a + t - t_a; // 合力のa方向成分の長さを|a|にする
            }
        }
        else
        {
            m = t;
        }

        motor::move(m.deg(), m.length());
    }
    else
    {
        motor::stay();
    }
}