#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/const_number.hpp"

namespace line
{
    // 通常の送受信データ内容
    struct t_data
    {
    } __attribute__((packed));
    struct r_data
    {
        uint32_t angel = 0UL;
        int16_t right_side_val = 0;
        int16_t left_side_val = 0;
    } __attribute__((packed));
    inline serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

    uint32_t angel = 0UL;                 // エンジェル
    uint8_t angel_count = 0;              // エンジェルの反応数
    float angel_x = 0.0f, angel_y = 0.0f; // エンジェルのx,y
    int16_t angel_deg = 0, angel_dis = 0; // エンジェルの角度,距離

    int16_t right_side_val = 0; // 右サイド
    int16_t left_side_val = 0;  // 左サイド

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process() // STM32との通信
    {
        // STM32のデータを受送信
        bool received = packet.update();

        // r_data代入
        if (received)
        {
            angel = packet.rx.angel;

            angel_count = 0;
            for (int i = 0; i < 32; i++)
            {
                if (angel & (1UL << i))
                {
                    angel_count++;
                    angel_x += LINEANGEL_RADIOUS * cosf(radians(360.0f / 32 * i));
                    angel_y += LINEANGEL_RADIOUS * sinf(radians(360.0f / 32 * i));
                }
            }

            if (angel_count > 0)
            {
                angel_x /= angel_count;
                angel_y /= angel_count;
                angel_deg = degrees(atan2f(angel_y, angel_x));
                angel_dis = sqrtf(angel_x * angel_x + angel_y * angel_y);
            }
            else
            {
                angel_x = 0.0f;
                angel_y = 0.0f;
                angel_deg = UNDETECTED;
                angel_dis = UNDETECTED;
            }

            right_side_val = packet.rx.right_side_val;
            left_side_val = packet.rx.left_side_val;
        }
    }
}