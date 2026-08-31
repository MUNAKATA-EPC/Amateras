#pragma once

#include <Arduino.h>

/*

namespace motordriver
{
    // PWMのAuto-Reload Register値 (duty分解能 = ARR+1 = 180段階)
    // 180MHz / 180 = 1MHz PWM周波数
    static constexpr uint32_t PWM_ARR = 179;
    static constexpr int16_t MP_MAX = 1000;

    struct pins
    {
        uint8_t in1; // EN/IN1 PWMピン
        uint8_t in2; // PH/IN2 方向ピン
    };

    inline static pins m[4];      // 各モーターのピン設定
    inline static int16_t m_p[4]; // 各モーターの目標出力 (-1000〜1000)

    namespace detail
    {
        inline uint32_t power_to_ccr(int16_t power)
        {
            int32_t abs_p = (power < 0) ? -power : power;
            if (abs_p > MP_MAX)
                abs_p = MP_MAX;

            return static_cast<uint32_t>(abs_p * PWM_ARR / MP_MAX);
        }

        inline void drive_channel(uint8_t ch)
        {
            const int16_t p = m_p[ch];
            const uint8_t en = m[ch].in1; // EN/IN1 PWMピン
            const uint8_t ph = m[ch].in2; // PH/IN2 方向ピン

            if (p == 0)
            {
                analogWrite(en, 0);
                digitalWrite(ph, LOW);
            }
            else if (p > 0)
            {
                digitalWrite(ph, HIGH);
                uint32_t ccr = power_to_ccr(p);
                uint32_t duty_12bit = ccr * 4095u / PWM_ARR;
                analogWrite(en, static_cast<int>(duty_12bit));
            }
            else
            {
                digitalWrite(ph, LOW);
                uint32_t ccr = power_to_ccr(p);
                uint32_t duty_12bit = ccr * 4095u / PWM_ARR;
                analogWrite(en, static_cast<int>(duty_12bit));
            }
        }
    }

    inline void attach(pins m1, pins m2, pins m3, pins m4)
    {
        m[0] = m1;
        m[1] = m2;
        m[2] = m3;
        m[3] = m4;

        for (int i = 0; i < 4; i++)
        {
            // EN/IN1 PWM出力ピン
            pinMode(m[i].in1, OUTPUT);
            // PH/IN2 デジタル出力ピン
            pinMode(m[i].in2, OUTPUT);

            analogWrite(m[i].in1, 0);
            digitalWrite(m[i].in2, LOW);
        }
    }

    inline void move(int16_t m1_p, int16_t m2_p, int16_t m3_p, int16_t m4_p)
    {
        m_p[0] = m1_p;
        m_p[1] = m2_p;
        m_p[2] = m3_p;
        m_p[3] = m4_p;
    }

    inline void process()
    {
        for (int i = 0; i < 4; i++)
        {
            detail::drive_channel(static_cast<uint8_t>(i));
        }
    }

    inline void stop_all()
    {
        for (int i = 0; i < 4; i++)
        {
            analogWrite(m[i].in1, 0);
            digitalWrite(m[i].in2, LOW);
            m_p[i] = 0;
        }
    }

}

*/

namespace motordriver
{
    // PWMのAuto-Reload Register値 (duty分解能 = ARR+1 = 180段階)
    // 180MHz / 180 = 1MHz PWM周波数
    static constexpr uint32_t PWM_ARR = 179;
    static constexpr int16_t MP_MAX = 1000;

    struct pins
    {
        uint8_t in1; // IN1
        uint8_t in2; // IN2
    };

    inline static pins m[4];                                          // 各モーターのピン設定
    inline static int16_t m_p[4];                                     // 各モーターの目標出力 (-1000〜1000)
    inline static int16_t m_last_p[4] = {-9999, -9999, -9999, -9999}; // 前回値保持用（変更検知）

    namespace detail
    {
        inline uint32_t power_to_duty12bit(int16_t power)
        {
            int32_t abs_p = (power < 0) ? -power : power;
            if (abs_p > MP_MAX)
                abs_p = MP_MAX;

            uint32_t ccr = static_cast<uint32_t>(abs_p * PWM_ARR / MP_MAX);
            return ccr * 4095u / PWM_ARR;
        }

        inline void drive_channel(uint8_t ch)
        {
            const int16_t p = m_p[ch];

            if (p == m_last_p[ch])
            {
                return;
            }
            m_last_p[ch] = p;

            const uint8_t in1 = m[ch].in1;
            const uint8_t in2 = m[ch].in2;

            if (p == 0)
            {
                analogWrite(in1, 0);
                analogWrite(in2, 0);
            }
            else if (p > 0)
            {
                // 正転 IN1にPWMを印加、IN2は0 (LOW)
                uint32_t duty = power_to_duty12bit(p);
                analogWrite(in2, 0);
                analogWrite(in1, static_cast<int>(duty));
            }
            else
            {
                // 逆転 IN1は0 (LOW)、IN2にPWMを印加
                uint32_t duty = power_to_duty12bit(p);
                analogWrite(in1, 0);
                analogWrite(in2, static_cast<int>(duty));
            }
        }
    }

    inline void attach(pins m1, pins m2, pins m3, pins m4)
    {
        m[0] = m1;
        m[1] = m2;
        m[2] = m3;
        m[3] = m4;

        for (int i = 0; i < 4; i++)
        {
            pinMode(m[i].in1, OUTPUT);
            pinMode(m[i].in2, OUTPUT);

            analogWrite(m[i].in1, 0);
            analogWrite(m[i].in2, 0);

            m_p[i] = 0;
            m_last_p[i] = -9999;
        }
    }

    inline void move(int16_t m1_p, int16_t m2_p, int16_t m3_p, int16_t m4_p)
    {
        m_p[0] = m1_p;
        m_p[1] = m2_p;
        m_p[2] = m3_p;
        m_p[3] = m4_p;
    }

    inline void process()
    {
        for (int i = 0; i < 4; i++)
        {
            detail::drive_channel(static_cast<uint8_t>(i));
        }
    }

    inline void stop_all()
    {
        for (int i = 0; i < 4; i++)
        {
            m_p[i] = 0;
            detail::drive_channel(static_cast<uint8_t>(i));
        }
    }

}