#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/const_number.hpp"

namespace camera
{
    // 通常の送受信データ内容
    struct t_data
    {
    } __attribute__((packed));
    struct r_data
    {
        int16_t ball_deg = UNDETECTED;
        int16_t ball_dis = UNDETECTED;
        int16_t yellow_goal_deg = UNDETECTED;
        int16_t yellow_goal_dis = UNDETECTED;
        int16_t blue_goal_deg = UNDETECTED;
        int16_t blue_goal_dis = UNDETECTED;
    } __attribute__((packed));
    inline serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

    int16_t ball_deg = UNDETECTED;
    int16_t ball_dis = UNDETECTED;
    int16_t yellow_goal_deg = UNDETECTED;
    int16_t yellow_goal_dis = UNDETECTED;
    int16_t blue_goal_deg = UNDETECTED;
    int16_t blue_goal_dis = UNDETECTED;

    int16_t offence_goal_deg = UNDETECTED;
    int16_t offence_goal_dis = UNDETECTED;
    int16_t defence_goal_deg = UNDETECTED;
    int16_t defence_goal_dis = UNDETECTED;

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process(int8_t meter_type) // STM32との通信
    {
        // STM32のデータを受送信
        packet.update();

        // r_data代入
        ball_deg = packet.rx.ball_deg;
        ball_dis = packet.rx.ball_dis;
        yellow_goal_deg = packet.rx.yellow_goal_deg;
        yellow_goal_dis = packet.rx.yellow_goal_dis;
        blue_goal_deg = packet.rx.blue_goal_deg;
        blue_goal_dis = packet.rx.blue_goal_dis;

        // offence・defence
        if (meter_type == 0)
        {
            offence_goal_deg = yellow_goal_deg;
            offence_goal_dis = yellow_goal_dis;
            defence_goal_deg = blue_goal_deg;
            defence_goal_dis = blue_goal_dis;
        }
        else if (meter_type == 1)
        {
            offence_goal_deg = blue_goal_deg;
            offence_goal_dis = blue_goal_dis;
            defence_goal_deg = yellow_goal_deg;
            defence_goal_dis = yellow_goal_dis;
        }
        else // if (meter_type == 2 || meter_type == -1)
        {
            offence_goal_deg = yellow_goal_deg;
            offence_goal_dis = yellow_goal_dis;
            defence_goal_deg = blue_goal_deg;
            defence_goal_dis = blue_goal_dis;
        }
    }
}