#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"

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
        int16_t ball_deg = 0;
        int16_t ball_dis = 0;
        int16_t gyro_deg = 0;
        int16_t yellow_goal_deg = 0;
        int16_t yellow_goal_dis = 0;
        int16_t blue_goal_deg = 0;
        int16_t blue_goal_dis = 0;
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

    // アクションが起動中の場合の送受信データ内容
    struct action_run_t_data
    {
        bool action_run = false;
        int16_t my_posi_x = 0;
        int16_t my_posi_y = 0;
    } __attribute__((packed));
    struct action_run_r_data
    {
        bool action_run = false;
        int16_t my_posi_x = 0;
        int16_t my_posi_y = 0;
    } __attribute__((packed));

    inline STATE cur_state = STATE::HOME;                                         // 現在のステート
    inline serial_packet<t_data, r_data> packet;                                  // 通常時の送受信パケット
    inline serial_packet<action_run_t_data, action_run_r_data> action_run_packet; // アクションが起動中の送受信パケット

    namespace ACTION
    {
        inline bool run = false;   // アクションが起動中かどうか
        inline int meter_type = 0; // 0:yellow,1:blue,2:gyro
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
        inline bool toggle = false; // testmotorで      true:run,   false:idle
        inline int meter_type = 0;  // 0:yellow,1:blue,2:gyro
    }

    inline bool isActionState(STATE state) // アクションが選択中かどうか
    {
        return (state == STATE::ACTION_OFFENCE || state == STATE::ACTION_DEFENCE || state == STATE::ACTION_RADIOCONTROL);
    }

    inline void attach(HardwareSerial &serial_obj) // どのシリアルで通信するか紐づけ
    {
        packet.begin(serial_obj);
        action_run_packet.begin(serial_obj);
    }

    inline void process(bool action_toggle) // UIとの通信
    {
        bool prev_action_run = ACTION::run;

        if (ACTION::run)
        {
            action_run_packet.update();

            TEST_KICKER::btn = false;
            TEST_KICKER::front = false;
            TEST_DRIBBLER::toggle = false;
            TEST_DRIBBLER::front = false;
            TEST_MOTOR::toggle = false;
            TEST_MOTOR::meter_type = 0;

            action_run_packet.tx.action_run = (action_toggle == true);
            ACTION::run = action_run_packet.rx.action_run;
        }
        else
        {
            packet.update();

            cur_state = packet.rx.cur_state;
            TEST_KICKER::btn = packet.rx.testkicker_btn;
            TEST_KICKER::front = packet.rx.testkicker_front;
            TEST_DRIBBLER::toggle = packet.rx.testdribbler_toggle;
            TEST_DRIBBLER::front = packet.rx.testdribbler_front;
            TEST_MOTOR::toggle = packet.rx.testmotor_toggle;
            TEST_MOTOR::meter_type = packet.rx.testmotor_meter_type;

            packet.tx.action_run = (isActionState(cur_state) && (action_toggle == true));
            ACTION::run = packet.rx.action_run;
            ACTION::meter_type = packet.rx.action_meter_type;
        }

        if (ACTION::run && !prev_action_run)
        {
            action_run_packet.reset();
        }
        else if (!ACTION::run && prev_action_run)
        {
            packet.reset();
        }
    }
}