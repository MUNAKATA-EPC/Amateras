#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/const_number.hpp"

namespace ps3
{
    // 通常の送受信データ内容
    struct t_data
    {
    } __attribute__((packed));
    struct r_data
    {
        int8_t stick_lx, stick_ly, stick_rx, stick_ry;
        uint16_t buttons_data_bit_mask = 0;
    } __attribute__((packed));
    serial_packet<t_data, r_data> packet;

    int8_t stick_lx, stick_ly, stick_rx, stick_ry;
    int8_t stick_ldis, stick_rdis;
    int16_t ldeg, rdeg;
    uint16_t buttons_data_bit_mask = 0;

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process() // STM32との通信
    {
        // STM32のデータを受送信
        packet.update();

        // r_data代入
        stick_lx = packet.rx.stick_lx;
        stick_ly = packet.rx.stick_ly;
        stick_rx = packet.rx.stick_rx;
        stick_ry = packet.rx.stick_ry;
        buttons_data_bit_mask = packet.rx.buttons_data_bit_mask;

        // dis計算
        stick_ldis = static_cast<int8_t>(sqrt(float(stick_lx * stick_lx) + float(stick_ly * stick_ly)));
        stick_rdis = static_cast<int8_t>(sqrt(float(stick_rx * stick_rx) + float(stick_ry * stick_ry)));

        // deg計算
        ldeg = static_cast<int16_t>(atan2(stick_ly, stick_lx) * 180.0 / PI);
        rdeg = static_cast<int16_t>(atan2(stick_ry, stick_rx) * 180.0 / PI);
    }
}