#pragma once

#include <Arduino.h>
#include <math.h>

// common
#include "common/angle_asist.hpp"
#include "common/bus_instance.hpp"
#include "common/serial_packet.hpp"
#include "common/vector.hpp"
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
    const float move_step_limit = 25.0f; // 最大移動指令ベクトル変化量 / 10 ms

    constexpr float control_dt = 0.01f;
    const float omega_ref_max = 200.0f;       // 途中目標の最大角速度 [deg/s]
    const float omega_ref_alpha_max = 100.0f; // 途中目標の最大角加速度 [deg/s^2]

    struct pd_data
    {
        float p = 0.0f, d = 0.0f;
    };

    inline pd_data pd;
    inline int16_t p_pow = 0, d_pow = 0;

    inline vector move_vec;
    inline vector move_ref_vec;

    inline void process(const pd_data &pd_obj, int16_t cur_deg, int16_t target_deg)
    {
        // 途中目標の角度・角速度と、前回のセンサー角度は小数を保持する
        static float deg_ref = 0.0f;
        static float omega_ref = 0.0f;
        static float last_deg = 0.0f;
        static bool angle_ref_initialized = false;

        static const pd_data *last_pd_addr = nullptr;
        static uint32_t last_calc_time = 0;
        static bool has_calculated = false;

        pd = pd_obj;

        if (!cyclic_timer_10ms.called())
            return;

        uint32_t current_time = millis();
        bool is_time_gap = has_calculated && ((current_time - last_calc_time) > 15);

        if (!angle_ref_initialized)
        {
            deg_ref = degWrapFloat(static_cast<float>(cur_deg));
            omega_ref = 0.0f;
            last_deg = static_cast<float>(cur_deg);
            angle_ref_initialized = true;
        }

        // 1) 最終目標までの残り角度から、目標軌道の角速度を作る
        // 2) 角加速度上限で omega_ref を omega_cmd に近づける
        // 3) omega_ref を今回の実際の角度変化量 deg_ref_step に変換する
        float deg_ref_step = 0.0f;
        if (is_time_gap)
        {
            // 周期を取りこぼした場合は、固定 dt 前提の角速度を次周期へ持ち越さない
            omega_ref = 0.0f;
        }
        else
        {
            float remaining_deg = degErrorFloat(static_cast<float>(target_deg), deg_ref);
            float direction = 0.0f;
            if (remaining_deg > 0.0f)
                direction = 1.0f;
            else if (remaining_deg < 0.0f)
                direction = -1.0f;

            float braking_speed = sqrtf(2.0f * omega_ref_alpha_max * fabsf(remaining_deg));
            float omega_cmd = direction * fminf(omega_ref_max, braking_speed);

            float max_omega_ref_change = omega_ref_alpha_max * control_dt;
            float omega_change = omega_cmd - omega_ref;
            omega_change = constrain(omega_change, -max_omega_ref_change, max_omega_ref_change);
            omega_ref += omega_change;

            deg_ref_step = omega_ref * control_dt;

            // 離散時間で目標を1ステップ飛び越さないための終端ガード
            if (remaining_deg == 0.0f ||
                (remaining_deg > 0.0f && deg_ref_step > remaining_deg) ||
                (remaining_deg < 0.0f && deg_ref_step < remaining_deg))
            {
                deg_ref_step = remaining_deg;
                omega_ref = 0.0f;
            }

            deg_ref = degWrapFloat(deg_ref + deg_ref_step);
        }

        // 今回の角度測定変化量。degErrorFloatで角度の折り返しを処理する
        float measured_step = degErrorFloat(static_cast<float>(cur_deg), last_deg);

        // P項は途中目標と実角度の誤差、D項は目標と実際の1周期変化量の差
        float error = degErrorFloat(deg_ref, static_cast<float>(cur_deg));
        float p_raw = pd.p * error;
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
        has_calculated = true;

        // 移動指令ベクトルの更新（元の移動ランプ処理を維持）
        if (is_time_gap)
        {
            move_vec = vector(0.0f, 0.0f);
            move_ref_vec = vector(0.0f, 0.0f);
        }
        else
        {
            vector v_vec = move_ref_vec - move_vec;
            if (v_vec.length() > move_step_limit)
                v_vec = vector(v_vec.deg(), move_step_limit);

            move_vec = move_vec + v_vec;
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
        float angle = radians(deg);
        float c = cosf(angle + PI / 4.0f);
        float s = sinf(angle + PI / 4.0f);
        float wheel_gain = max(fabsf(c), fabsf(s));

        if (max_pow <= 0 || wheel_gain == 0.0f)
            move_ref_vec = vector(0.0f, 0.0f);
        else
            move_ref_vec = vector(static_cast<float>(deg),
                                  static_cast<float>(max_pow) / wheel_gain);

        constexpr float inv_sqrt2 = 0.70710678f;
        float x = move_vec.x();
        float y = move_vec.y();

        int16_t move_pow[4] = {
            static_cast<int16_t>((x - y) * inv_sqrt2),
            static_cast<int16_t>((x + y) * inv_sqrt2),
            static_cast<int16_t>((-x + y) * inv_sqrt2),
            static_cast<int16_t>((-x - y) * inv_sqrt2)};

        detail::movemain(move_pow);
    }

    inline void move(int16_t deg, int16_t pow)
    {
        move_ref_vec = vector(static_cast<float>(deg), static_cast<float>(pow));

        constexpr float inv_sqrt2 = 0.70710678f;
        float x = move_vec.x();
        float y = move_vec.y();

        int16_t move_pow[4] = {
            static_cast<int16_t>((x - y) * inv_sqrt2),
            static_cast<int16_t>((x + y) * inv_sqrt2),
            static_cast<int16_t>((-x + y) * inv_sqrt2),
            static_cast<int16_t>((-x - y) * inv_sqrt2)};

        detail::movemain(move_pow);
    }

    inline void stay()
    {
        move(0, 0);
    }
}

const motor::pd_data pd_gyro = {12.8f, 5.2f};
const motor::pd_data pd_cam = {12.8f, 5.2f};
