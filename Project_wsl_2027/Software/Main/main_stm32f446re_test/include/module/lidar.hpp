#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/const_number.hpp"

namespace lidar
{
    // 通常の送受信データ内容
    struct t_data
    {
    } __attribute__((packed));
    struct r_data
    {
        int16_t posi_x = 0;
        int16_t posi_y = 0;
    } __attribute__((packed));
    inline serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

    int16_t posi_x = 0;
    int16_t posi_y = 0;

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process() // STM32との通信
    {
        // STM32のデータを受送信
        packet.update();

        // r_data代入
        posi_x = packet.rx.posi_x;
        posi_y = packet.rx.posi_y;
    }
}