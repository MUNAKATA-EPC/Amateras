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
            float r = sqrtf((float)lidar::posi_x * (float)lidar::posi_x + (float)lidar::posi_y * (float)lidar::posi_y);

            motor::maximizemove(deg_wrap_float(posi_to_deg(-lidar::posi_x, -lidar::posi_y) - gyro.deg()), static_cast<int16_t>(r > 800 ? 800 : r));
        }
        else
        {
            motor::stay();
        }
    }
    else if (ps3::is_pushing(ps3::BUTTON::SQUARE))
    {
        static float r = 0.0f;
        if (!ps3::is_prev_pushing(ps3::BUTTON::SQUARE))
        {
            r = sqrtf((float)lidar::posi_x * (float)lidar::posi_x + (float)lidar::posi_y * (float)lidar::posi_y);
        }

        vector o = vector(0.0f, 0.0f, static_cast<float>(-lidar::posi_x), static_cast<float>(-lidar::posi_y));
        vector o_r = o * r / o.length();
        vector t = o - o_r;

        bool b1 = ps3::is_pushing(ps3::BUTTON::L1);
        bool b2 = ps3::is_pushing(ps3::BUTTON::R1);
        vector m;
        if (b1 || b2)
        {
            vector a = vector(0.0f, 0.0f, b1 ? -o.y() : o.y(), b1 ? o.x() : -o.x()) * 600.0f / o.length();

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

        motor::move(deg_wrap_float(m.deg() - gyro.deg()), m.length());
    }
    else
    {
        motor::stay();
    }
}