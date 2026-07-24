#pragma once

#include <Arduino.h>
#include "common/serial_packet.hpp"
#include "common/my_interface.hpp"
#include "button.hpp"

#define M5_SERIAL 
#define TOGGLE_PIN

enum UI_STATE : uint8_t
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
enum UI_STATE ui_state = HOME;

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
    UI_STATE ui_state = HOME;
    bool testkicker_btn = false;
    bool testkicker_front = false;
    bool testdribbler_toggle = false;
    bool testdribbler_front = false;
    bool testmotor_toggle = false;
    int8_t testmotor_meter_type = 0;
} __attribute__((packed));

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

serial_packet<t_data, r_data> packet;
serial_packet<action_run_t_data, action_run_r_data> action_run_packet;

bool action_run = false;
int action_meter_type = 0;
bool testkicker_btn = false;
bool testkicker_front = false;
bool testdribbler_toggle = false;
bool testdribbler_front = false;
bool testmotor_toggle = false;
int testmotor_meter_type = 0;

bool isActionState(UI_STATE state)
{
    return (state == ACTION_OFFENCE || state == ACTION_DEFENCE || state == ACTION_RADIOCONTROL);
}

int toggle_stable_judge(bool cur_toggle)
{
    static bool last_state = false;
    static uint8_t consecutive_count = 0;

    if (cur_toggle == last_state)
    {
        if (consecutive_count < 5)
        {
            consecutive_count++;
        }
    }
    else
    {
        last_state = cur_toggle;
        consecutive_count = 1;
    }

    if (consecutive_count >= 5)
    {
        return cur_toggle ? 1 : 0;
    }

    return -1;
}

uint8_t toggle_pin = PB14;

void uiInit()
{
    mySerial3.begin(115200);
    packet.begin(mySerial3);
    action_run_packet.begin(mySerial3);
}

void uiProcess()
{
    // トグルスイッチ
    static bool prev_action_run = false;
    int action_toggle = toggle_stable_judge(digitalRead(toggle_pin) == HIGH);

    // アップデート
    static uint32_t last_time = 0;
    if (millis() - last_time > 20)
    {
        bool prev_action_run = action_run;

        // M5のデータを受送信
        if (action_run)
        {
            action_run_packet.update();

            testkicker_btn = false;
            testkicker_front = false;
            testdribbler_toggle = false;
            testdribbler_front = false;
            testmotor_toggle = false;
            testmotor_meter_type = 0;

            // アクションを走らせるか
            action_run_packet.tx.action_run = (action_toggle == 1);
            action_run = action_run_packet.rx.action_run;
        }
        else
        {
            packet.update();

            ui_state = packet.rx.ui_state;
            testkicker_btn = packet.rx.testkicker_btn;
            testkicker_front = packet.rx.testkicker_front;
            testdribbler_toggle = packet.rx.testdribbler_toggle;
            testdribbler_front = packet.rx.testdribbler_front;
            testmotor_toggle = packet.rx.testmotor_toggle;
            testmotor_meter_type = packet.rx.testmotor_meter_type;

            // アクションを走らせるか
            packet.tx.action_run = (isActionState(ui_state) && (action_toggle == 1));
            action_run = packet.rx.action_run;
            action_meter_type = packet.rx.action_meter_type;
        }

        // 切替時にシリアルバッファをリセット
        if (action_run && !prev_action_run)
        {
            action_run_packet.reset();
        }
        else if (!action_run && prev_action_run)
        {
            packet.reset();
        }

        last_time = millis();
    }
}