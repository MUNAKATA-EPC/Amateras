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
    const int16_t pd_pow_limit = 500, move_pow_limit = 990;

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
            int16_t error = deg_error(target_deg, cur_deg);

            bool is_new_pd_obj = (&pd_obj != last_pd_addr);
            bool is_time_gap = ((current_time - last_calc_time) > 15);

            if (is_new_pd_obj || is_time_gap)
            {
                d_pow = 0;
            }
            else
            {
                int16_t d_diff = cur_deg - last_deg;
                d_pow = constrain(static_cast<int16_t>(static_cast<float>(-d_diff) * pd.d), -pd_pow_limit, pd_pow_limit);
            }

            p_pow = constrain(static_cast<int16_t>(static_cast<float>(error) * pd.p), -pd_pow_limit, pd_pow_limit);

            last_deg = cur_deg;
            last_pd_addr = &pd_obj;
            last_calc_time = current_time;
        }
    }

    namespace detail
    {
        inline void movemain(int16_t *move_pow)
        {
            int16_t total_pd_pow = constrain(p_pow + d_pow, -pd_pow_limit, pd_pow_limit);
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

            if (max_total_abs_pow > move_pow_limit)
            {
                float scale = (static_cast<float>(move_pow_limit - abs(total_pd_pow))) / static_cast<float>(abs(move_pow[max_total_abs_pow_index]));

                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] = static_cast<int16_t>(static_cast<float>(move_pow[i]) * scale) + total_pd_pow;

                    if (move_pow[i] > move_pow_limit)
                        move_pow[i] = move_pow_limit;
                    else if (move_pow[i] < -move_pow_limit)
                        move_pow[i] = -move_pow_limit;
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] += total_pd_pow;

                    if (move_pow[i] > move_pow_limit)
                        move_pow[i] = move_pow_limit;
                    else if (move_pow[i] < -move_pow_limit)
                        move_pow[i] = -move_pow_limit;
                }
            }

            motordriver::move(move_pow[0], move_pow[1], move_pow[2], move_pow[3]);
        }
    }

    inline void maximizemove(int16_t deg, int16_t max_pow)
    {
        float c = cos(radians(deg + 45));
        float s = sin(radians(deg + 45));
        int16_t move_pow[4] = {static_cast<int16_t>(static_cast<float>(max_pow) * c),
                               static_cast<int16_t>(static_cast<float>(max_pow) * s),
                               static_cast<int16_t>(-static_cast<float>(max_pow) * c),
                               static_cast<int16_t>(-static_cast<float>(max_pow) * s)};

        int max_abs_move_pow = 0;
        for (int i = 0; i < 4; i++)
        {
            if (abs(move_pow[i]) > max_abs_move_pow)
                max_abs_move_pow = abs(move_pow[i]);
        }

        float scale = static_cast<float>(max_pow) / static_cast<float>(max_abs_move_pow);
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
        int16_t move_pow[4] = {static_cast<int16_t>(static_cast<float>(pow) * c),
                               static_cast<int16_t>(static_cast<float>(pow) * s),
                               static_cast<int16_t>(-static_cast<float>(pow) * c),
                               static_cast<int16_t>(-static_cast<float>(pow) * s)};

        detail::movemain(move_pow);
    }

    inline void stay()
    {
        move(0, 0);
    }
}

const motor::pd_data pd_gyro = {12.8f, 5.2f};
const motor::pd_data pd_cam = {12.8f, 5.2f};

#ifdef V2

#pragma once

#include <Arduino.h>
#include <math.h>
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
    const int16_t pd_pow_limit = 500, move_pow_limit = 990;

    struct pd_data
    {
        float p = 0.0f, d = 0.0f;
    };

    inline pd_data pd;
    inline int16_t p_pow = 0, d_pow = 0;

    // 角度目標軌道の上限。単位は omega_ref_max: deg/s,
    // omega_ref_alpha_max: deg/s^2。
    constexpr float control_dt = 0.01f;
    const float omega_ref_max = 200.0f;
    const float omega_ref_alpha_max = 100.0f;

    inline void process(const pd_data &pd_obj, int16_t cur_deg, int16_t target_deg)
    {
        // deg_ref / omega_ref: 途中目標の角度と角速度。
        static float deg_ref = 0.0f;
        static float omega_ref = 0.0f;
        static float last_deg = 0.0f;
        static bool ref_initialized = false;

        static const pd_data *last_pd_addr = nullptr;
        static uint32_t last_calc_time = 0;

        pd = pd_obj;

        if (!cyclic_timer_10ms.called())
            return;

        uint32_t current_time = millis();
        bool is_time_gap = ((current_time - last_calc_time) > 15);

        if (!ref_initialized)
        {
            deg_ref = static_cast<float>(cur_deg);
            omega_ref = 0.0f;
            last_deg = static_cast<float>(cur_deg);
            ref_initialized = true;
        }

        // float角度用の最短角度差。既存deg_error()と同じ角度折り返しを扱う。
        auto deg_error_float = [](float target, float current) -> float
        {
            float error = fmodf(target - current, 360.0f);
            if (error > 180.0f)
                error -= 360.0f;
            else if (error < -180.0f)
                error += 360.0f;
            return error;
        };

        auto wrap_deg = [](float deg) -> float
        {
            float wrapped = fmodf(deg, 360.0f);
            if (wrapped < 0.0f)
                wrapped += 360.0f;
            return wrapped;
        };

        // 目標角度までの残りと、この周期に途中目標を進める量。
        float remaining_deg = deg_error_float(static_cast<float>(target_deg), deg_ref);
        float deg_ref_step = 0.0f;

        if (is_time_gap)
        {
            // 10 ms周期から外れた場合、固定周期前提の速度を持ち越さない。
            omega_ref = 0.0f;
        }
        else
        {
            float direction = 0.0f;
            if (remaining_deg > 0.0f)
                direction = 1.0f;
            else if (remaining_deg < 0.0f)
                direction = -1.0f;

            // 残り角度で停止できる角速度を計算し、最大角速度で制限する。
            float braking_speed = sqrtf(2.0f * omega_ref_alpha_max * fabsf(remaining_deg));
            float omega_cmd = direction * fminf(omega_ref_max, braking_speed);

            // omega_refの変化を角加速度上限内に制限する。
            float max_omega_change = omega_ref_alpha_max * control_dt;
            float omega_change = constrain(omega_cmd - omega_ref,
                                           -max_omega_change, max_omega_change);
            omega_ref += omega_change;

            // 目標角速度 [deg/s] を1周期の角度差分 [deg/tick] に変換する。
            deg_ref_step = omega_ref * control_dt;

            // 離散時間で最終目標を飛び越さないよう、最後の1ステップを制限する。
            if (remaining_deg == 0.0f ||
                (remaining_deg > 0.0f && deg_ref_step > remaining_deg) ||
                (remaining_deg < 0.0f && deg_ref_step < remaining_deg))
            {
                deg_ref_step = remaining_deg;
                omega_ref = 0.0f;
            }

            deg_ref = wrap_deg(deg_ref + deg_ref_step);
        }

        // 実角度がこの10 msで変化した量。deg_error()と同じ折り返しを使う。
        float measured_step = deg_error_float(static_cast<float>(cur_deg), last_deg);

        // P項は途中目標との角度誤差。
        float error = deg_error_float(deg_ref, static_cast<float>(cur_deg));
        float p_raw = pd.p * error;

        // D項は「目標の角度変化 - 実際の角度変化」。
        // deg_step単位なので、既存のpd.dをそのまま使う。
        float d_diff = deg_ref_step - measured_step;
        float d_raw = pd.d * d_diff;

        bool is_new_pd_obj = (&pd_obj != last_pd_addr);
        if (is_new_pd_obj || is_time_gap)
        {
            d_pow = 0;
        }
        else
        {
            d_pow = static_cast<int16_t>(constrain(
                d_raw, -static_cast<float>(pd_pow_limit), static_cast<float>(pd_pow_limit)));
        }

        p_pow = static_cast<int16_t>(constrain(
            p_raw, -static_cast<float>(pd_pow_limit), static_cast<float>(pd_pow_limit)));

        last_deg = static_cast<float>(cur_deg);
        last_pd_addr = &pd_obj;
        last_calc_time = current_time;

        // 移動・車輪合成は元の動作を維持する。
    }

    namespace detail
    {
        inline void movemain(int16_t *move_pow)
        {
            int16_t total_pd_pow = constrain(p_pow + d_pow, -pd_pow_limit, pd_pow_limit);
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

            if (max_total_abs_pow > move_pow_limit)
            {
                float scale = (static_cast<float>(move_pow_limit - abs(total_pd_pow))) /
                              static_cast<float>(abs(move_pow[max_total_abs_pow_index]));

                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] = static_cast<int16_t>(static_cast<float>(move_pow[i]) * scale) + total_pd_pow;

                    if (move_pow[i] > move_pow_limit)
                        move_pow[i] = move_pow_limit;
                    else if (move_pow[i] < -move_pow_limit)
                        move_pow[i] = -move_pow_limit;
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    move_pow[i] += total_pd_pow;

                    if (move_pow[i] > move_pow_limit)
                        move_pow[i] = move_pow_limit;
                    else if (move_pow[i] < -move_pow_limit)
                        move_pow[i] = -move_pow_limit;
                }
            }

            motordriver::move(move_pow[0], move_pow[1], move_pow[2], move_pow[3]);
        }
    }

    inline void maximizemove(int16_t deg, int16_t max_pow)
    {
        float c = cos(radians(deg + 45));
        float s = sin(radians(deg + 45));
        int16_t move_pow[4] = {static_cast<int16_t>(static_cast<float>(max_pow) * c),
                               static_cast<int16_t>(static_cast<float>(max_pow) * s),
                               static_cast<int16_t>(-static_cast<float>(max_pow) * c),
                               static_cast<int16_t>(-static_cast<float>(max_pow) * s)};

        int max_abs_move_pow = 0;
        for (int i = 0; i < 4; i++)
        {
            if (abs(move_pow[i]) > max_abs_move_pow)
                max_abs_move_pow = abs(move_pow[i]);
        }

        float scale = static_cast<float>(max_pow) / static_cast<float>(max_abs_move_pow);
        for (int i = 0; i < 4; i++)
            move_pow[i] = static_cast<int16_t>(static_cast<float>(move_pow[i]) * scale);

        detail::movemain(move_pow);
    }

    inline void move(int16_t deg, int16_t pow)
    {
        float c = cos(radians(deg + 45));
        float s = sin(radians(deg + 45));
        int16_t move_pow[4] = {static_cast<int16_t>(static_cast<float>(pow) * c),
                               static_cast<int16_t>(static_cast<float>(pow) * s),
                               static_cast<int16_t>(-static_cast<float>(pow) * c),
                               static_cast<int16_t>(-static_cast<float>(pow) * s)};

        detail::movemain(move_pow);
    }

    inline void stay()
    {
        move(0, 0);
    }
}

const motor::pd_data pd_gyro = {12.8f, 5.2f};
const motor::pd_data pd_cam = {12.8f, 5.2f};

#endif