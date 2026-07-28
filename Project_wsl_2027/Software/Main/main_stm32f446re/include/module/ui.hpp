#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/timer.hpp"
#include "common/const_number.hpp"

namespace ui
{
    // ステート
    enum class STATE : uint8_t
    {
        HOME,
        ACTION_OFFENCE,
        ACTION_DEFENCE,
        ACTION_RADIOCONTROL,
        TEST_KICKER,
        TEST_DRIBBLER,
        TEST_MOTOR,
        SENSORMONITOR_BALL,
        SENSORMONITOR_LINE,
        SENSORMONITOR_GYRO,
        SENSORMONITOR_GOAL,
        SENSORMONITOR_LIDAR,
        COMMUNICATION_TRANSMIT,
        COMMUNICATION_RECEIVE
    };

    // 通常の送受信データ内容
    struct t_data
    {
        bool action_run = false;
        uint32_t line_angel = 0UL;
        int16_t line_right_side_val = 0;
        int16_t line_left_side_val = 0;
        int16_t ball_deg = UNDETECTED;
        int16_t ball_dis = UNDETECTED;
        int16_t gyro_deg = UNDETECTED;
        int16_t yellow_goal_deg = UNDETECTED;
        int16_t yellow_goal_dis = UNDETECTED;
        int16_t blue_goal_deg = UNDETECTED;
        int16_t blue_goal_dis = UNDETECTED;
    } __attribute__((packed));
    struct r_data
    {
        bool action_run = false;
        int8_t action_meter_type = 0;
        STATE cur_state = STATE::HOME;
        bool testkicker_btn = false;
        bool testkicker_front = false;
        bool testdribbler_toggle = false;
        bool testdribbler_front = false;
        bool testmotor_toggle = false;
        int8_t testmotor_meter_type = 0;
    } __attribute__((packed));
    inline STATE cur_state = STATE::HOME;        // 現在のステート
    inline serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

    namespace ACTION
    {
        inline bool run = false;      // アクションが起動中かどうか
        inline bool last_run = false; // アクションが起動中かどうか
        inline int8_t meter_type = 0; // 0:yellow,1:blue,2:gyro
    }
    namespace TEST_KICKER
    {
        inline bool btn = false;   // testkicker_btnで true:kick,  false:idle
        inline bool front = false; // testkickerで     true:front, false:rear
    }
    namespace TEST_DRIBBLER
    {
        inline bool toggle = false; // testdribblerで   true:run,   false:idle
        inline bool front = false;  // testdribblerで   true:front, false:rear
    }
    namespace TEST_MOTOR
    {
        inline bool toggle = false;   // testmotorで      true:run,   false:idle
        inline int8_t meter_type = 0; // 0:yellow,1:blue,2:gyro
    }

    inline bool isActionState(STATE state) // アクションが選択中かどうか
    {
        return (state == STATE::ACTION_OFFENCE || state == STATE::ACTION_DEFENCE || state == STATE::ACTION_RADIOCONTROL);
    }

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
    }

    inline void process(bool toggle) // M5Stackとの通信
    {
        // ACTION::runを更新
        ACTION::run = (isActionState(cur_state) && (toggle == true));

        // M5Stackのデータを受送信 // ACTION::run中はpacketの更新は行わない
        static timer run_wait;
        if (ACTION::run)
        {
            if (!ACTION::last_run)
            {
                packet.update();

                run_wait.reset();
            }
            else if (run_wait.everReset() && run_wait.msTime() < 100)
            {
                packet.update();
            }
        }
        else
        {
            packet.update();
        }
        ACTION::last_run = ACTION::run;

        // t_data代入
        packet.tx.action_run = ACTION::run;
        // r_data代入
        cur_state = packet.rx.cur_state;
        ACTION::meter_type = packet.rx.action_meter_type;
        TEST_KICKER::btn = packet.rx.testkicker_btn;
        TEST_KICKER::front = packet.rx.testkicker_front;
        TEST_DRIBBLER::toggle = packet.rx.testdribbler_toggle;
        TEST_DRIBBLER::front = packet.rx.testdribbler_front;
        TEST_MOTOR::toggle = packet.rx.testmotor_toggle;
        TEST_MOTOR::meter_type = packet.rx.testmotor_meter_type;
    }
}