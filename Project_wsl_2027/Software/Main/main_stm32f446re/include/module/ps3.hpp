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

    int16_t stick_lx, stick_ly, stick_rx, stick_ry;
    int16_t stick_ldis, stick_rdis;
    int16_t stick_ldeg, stick_rdeg;
    // ボタン
    uint16_t buttons_data_bit_mask = 0;
    enum class BUTTON : uint8_t
    {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        TRIANGLE,
        CIRCLE,
        CROSS,
        SQUARE,
        L1,
        L2,
        L3,
        R1,
        R2,
        R3
    };

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process() // STM32との通信
    {
        // STM32のデータを受送信
        packet.update();

        // r_data代入
        stick_lx = static_cast<int16_t>(packet.rx.stick_lx);
        stick_ly = -static_cast<int16_t>(packet.rx.stick_ly);
        stick_rx = static_cast<int16_t>(packet.rx.stick_rx);
        stick_ry = -static_cast<int16_t>(packet.rx.stick_ry);
        buttons_data_bit_mask = packet.rx.buttons_data_bit_mask;

        // dis計算
        stick_ldis = static_cast<int16_t>(constrain(sqrt(static_cast<float>(stick_lx) * static_cast<float>(stick_lx) + static_cast<float>(stick_ly) * static_cast<float>(stick_ly)), 0.0f, 128.0f));
        stick_rdis = static_cast<int16_t>(constrain(sqrt(static_cast<float>(stick_rx) * static_cast<float>(stick_rx) + static_cast<float>(stick_ry) * static_cast<float>(stick_ry)), 0.0f, 128.0f));

        // deg計算
        stick_ldeg = static_cast<int16_t>(atan2(static_cast<float>(stick_lx), static_cast<float>(stick_ly)) * 180.0 / PI);
        stick_rdeg = static_cast<int16_t>(atan2(static_cast<float>(stick_rx), static_cast<float>(stick_ry)) * 180.0 / PI);
    }

    bool is_pushing(BUTTON btn)
    {
        return ((1 << (int)btn) & buttons_data_bit_mask) > 0;
    }
}