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
        int16_t gyro_deg = 0;
    } __attribute__((packed));
    struct r_data
    {
        int16_t x = 0, y = 0;
        int16_t enemy_x = 0, enemy_y = 0;
    } __attribute__((packed));
    inline serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

    int16_t posi_x = 0;
    int16_t posi_y = 0;
    int16_t posi_enemy_x = 0;
    int16_t posi_enemy_y = 0;

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process(int16_t gyro_deg) // RP2350との通信
    {
        // RP2350のデータを受送信
        packet.update();

        // t_data代入
        packet.tx.gyro_deg = gyro_deg;

        // r_data代入
        posi_x = packet.rx.x;
        posi_y = packet.rx.y;
        posi_enemy_x = packet.rx.enemy_x;
        posi_enemy_y = packet.rx.enemy_y;
    }
}