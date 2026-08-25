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

    uint32_t angel = 0UL;       // エンジェル
    int16_t right_side_val = 0; // 右サイド
    int16_t left_side_val = 0;  // 左サイド

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process() // STM32との通信
    {
        // STM32のデータを受送信
        packet.update();

        // r_data代入
        angel = packet.rx.angel;
        right_side_val = packet.rx.right_side_val;
        left_side_val = packet.rx.left_side_val;
    }
}