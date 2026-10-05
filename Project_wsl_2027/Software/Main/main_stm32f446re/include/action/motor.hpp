#pragma once

#include <Arduino.h>
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
// module
#include "module/camera.hpp"
#include "module/lidar.hpp"
#include "module/line.hpp"
#include "module/motordriver.hpp"
#include "module/ui.hpp"

namespace motor
{
    struct pd_data
    {
        float p = 0.0f, d = 0.0f;
    };

    inline pd_data pd;
    inline int16_t p_pow = 0, d_pow = 0;

    inline void process(const pd_data &pd_obj, int16_t cur_deg, int16_t target_deg)
    {
        static int16_t last_deg = 0;
        static const pd_data *last_pd_addr = nullptr;
        static uint32_t last_calc_time = 0;

        pd = pd_obj;

        if (cyclic_timer_10ms.called())
        {
            uint32_t current_time = millis();
            int16_t error = degError(target_deg, cur_deg);

            bool is_new_pd_obj = (&pd_obj != last_pd_addr);
            bool is_time_gap = ((current_time - last_calc_time) > 15);

            if (is_new_pd_obj || is_time_gap)
            {
                d_pow = 0;
            }
            else
            {
                int16_t d_diff = cur_deg - last_deg;
                d_pow = static_cast<int16_t>(static_cast<float>(-d_diff) * pd.d);
            }

            p_pow = static_cast<int16_t>(static_cast<float>(error) * pd.p);

            last_deg = cur_deg;
            last_pd_addr = &pd_obj;
            last_calc_time = current_time;
        }
    }

    namespace detail
    {
        inline void movemain(int16_t *move_pow)
        {
            int16_t total_pd_pow = p_pow + d_pow;
            int16_t total_abs_pow[4] = {
                static_cast<int16_t>(abs(move_pow[0] + total_pd_pow)),
                static_cast<int16_t>(abs(move_pow[1] + total_pd_pow)),
                static_cast<int16_t>(abs(move_pow[2] + total_pd_pow)),
                static_cast<int16_t>(abs(move_pow[3] + total_pd_pow))};

            int max_total_abs_pow = 0;
            int max_total_abs_pow_index = 0;
            for (int i = 0; i < 4; i++)
            {
                if (total_abs_pow[i] > max_total_abs_pow)
                {
                    max_total_abs_pow = total_abs_pow[i];
                    max_total_abs_pow_index = i;
                }
            }

            if ((max_total_abs_pow > 1000))
            {
                float scale = (1000.0f - static_cast<float>(abs(total_pd_pow))) / static_cast<float>(abs(move_pow[max_total_abs_pow_index]));

                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] = static_cast<int16_t>(static_cast<float>(move_pow[i]) * scale) + total_pd_pow;

                    if (move_pow[i] > 1000)
                        move_pow[i] = 1000;
                    else if (move_pow[i] < -1000)
                        move_pow[i] = -1000;
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] += total_pd_pow;

                    if (move_pow[i] > 1000)
                        move_pow[i] = 1000;
                    else if (move_pow[i] < -1000)
                        move_pow[i] = -1000;
                }
            }

            motordriver::move(move_pow[0], move_pow[1], move_pow[2], move_pow[3]);
        }
    }

    inline void maximizemove(int16_t deg)
    {
        float c = cos(radians(deg + 45));
        float s = sin(radians(deg + 45));
        int16_t move_pow[4] = {static_cast<int16_t>(-1000.0f * c),
                               static_cast<int16_t>(-1000.0f * s),
                               static_cast<int16_t>(1000.0f * c),
                               static_cast<int16_t>(1000.0f * s)};

        int max_abs_move_pow = 0;
        for (int i = 0; i < 4; i++)
        {
            if (abs(move_pow[i]) > max_abs_move_pow)
                max_abs_move_pow = abs(move_pow[i]);
        }

        float scale = 1000.0f / static_cast<float>(max_abs_move_pow);
        for (int i = 0; i < 4; i++)
        {
            move_pow[i] = static_cast<int16_t>(static_cast<float>(move_pow[i]) * scale);
        }

        detail::movemain(move_pow);
    }

    inline void move(int16_t deg, int16_t pow)
    {
        float c = cos(radians(deg + 45));
        float s = sin(radians(deg + 45));
        int16_t move_pow[4] = {static_cast<int16_t>(-(float)pow * c),
                               static_cast<int16_t>(-(float)pow * s),
                               static_cast<int16_t>((float)pow * c),
                               static_cast<int16_t>((float)pow * s)};

        detail::movemain(move_pow);
    }

    inline void stay()
    {
        move(0, 0);
    }
}

const motor::pd_data pd_gyro = {0.8f, 0.05f};
const motor::pd_data pd_cam = {0.8f, 0.05f};