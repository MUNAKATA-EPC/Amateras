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

void offence()
{
    if (ps3::stick_rdis > 20)
    {
        motor::process(pd_gyro, gyro.deg(), ps3::stick_rdeg);
    }
    else
    {
        motor::process(pd_gyro, gyro.deg(), 0);
    }

    if (line::angel)
    {
        motor::maximizemove(deg_wrap_float(posi_to_deg(-lidar::posi_x, -lidar::posi_y) - gyro.deg()), 900);
    }
    else if (ps3::stick_ldis > 20)
    {
        if (ps3::is_pushing(ps3::BUTTON::CIRCLE))
        {
            motor::maximizemove(deg_wrap_float(ps3::stick_ldeg - gyro.deg()), static_cast<int16_t>(ps3::stick_ldis * 900 / 128));
        }
        else
        {
            motor::maximizemove(deg_wrap_float(ps3::stick_ldeg - gyro.deg()), static_cast<int16_t>(ps3::stick_ldis * 500 / 128));
        }
    }
    else if (ps3::is_pushing(ps3::BUTTON::TRIANGLE))
    {
        if (lidar::posi_x * lidar::posi_x + lidar::posi_y * lidar::posi_y > 100)
        {
            motor::maximizemove(deg_wrap_float(posi_to_deg(-lidar::posi_x, -lidar::posi_y) - gyro.deg()), 600);
        }
        else
        {
            motor::stay();
        }
    }
    else if (ps3::is_pushing(ps3::BUTTON::SQUARE))
    {
        float rr = 0.0f;
        if (!ps3::is_prev_pushing(ps3::BUTTON::SQUARE))
        {
            rr = (float)lidar::posi_x * (float)lidar::posi_x + (float)lidar::posi_y * (float)lidar::posi_y;
        }

        vector o = vector(0.0f, 0.0f, static_cast<float>(-lidar::posi_x), static_cast<float>(-lidar::posi_y));
        vector o_r = o * sqrtf(rr) / o.length();
        vector t = o - o_r;

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