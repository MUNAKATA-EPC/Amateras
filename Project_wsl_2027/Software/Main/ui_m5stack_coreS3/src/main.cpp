#include <lvgl.h>
#include <M5Unified.h>
#include "ui.h"
#include "my_lv_func.h"
#include "common/timer.hpp"
#include "sensor/serial_packet.hpp"
#include "common/vector.hpp"

static const uint32_t screen_width = 320;
static const uint32_t screen_height = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screen_width * 60];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.pushImage(area->x1, area->y1, w, h, (uint16_t *)color_p);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    auto count = M5.Touch.getCount();
    if (count > 0)
    {
        auto t = M5.Touch.getDetail(0);
        data->state = LV_INDEV_STATE_PR;
        data->point.x = t.x;
        data->point.y = t.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

struct t_data
{
    bool action_run = false;
    int action_meter_type = 0;
    UI_STATE ui_state = HOME;
    bool testkicker_btn = false;
    bool testkicker_front = false;
    bool testdribbler_toggle = false;
    bool testdribbler_front = false;
    bool testmotor_toggle = false;
    int testmotor_meter_type = 0;

} __attribute__((packed));

struct r_data
{
    bool action_run = false;
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
bool last_action_run = false;
bool just_action_run_started = false;
bool just_action_run_stopped = false;

bool isActionState(UI_STATE state)
{
    return (state == ACTION_OFFENSE || state == ACTION_DEFENSE || state == ACTION_RADIOCONTROL);
}

void setup()
{
    Serial.begin(115200);

    Serial0.begin(115200);
    packet.begin(Serial0);
    action_run_packet.begin(Serial0);

    auto cfg = M5.config();
    M5.begin(cfg);

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screen_width * 60);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screen_width;
    disp_drv.ver_res = screen_height;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
}

void loop()
{
    // アップデート
    static uint32_t last_time = 0;
    if (millis() - last_time > 20)
    {
        // STMのデータを受送信
        if (action_run)
        {
            action_run_packet.update();

            // アクション応答
            action_run_packet.tx.action_run = action_run_packet.rx.action_run;
            action_run = action_run_packet.rx.action_run;
        }
        else
        {
            packet.update();

            // ui_stateによる場合分けはしていない
            packet.tx.ui_state = ui_state;
            if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_1))
            {
                packet.tx.testmotor_meter_type = 0;
            }
            else if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_2))
            {
                packet.tx.testmotor_meter_type = 1;
            }
            else // if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_3))
            {
                packet.tx.testmotor_meter_type = 2;
            }
            packet.tx.testkicker_btn = lv_obj_has_state(ui_TestKickerKickButton, LV_STATE_PRESSED);
            packet.tx.testkicker_front = !lv_obj_has_flag(ui_TestKickerLeverFrontButton, LV_OBJ_FLAG_HIDDEN);
            packet.tx.testdribbler_toggle = !lv_obj_has_flag(ui_TestDribblerLeverRunButton, LV_OBJ_FLAG_HIDDEN);
            packet.tx.testdribbler_front = !lv_obj_has_flag(ui_TestDribblerLeverFrontButton, LV_OBJ_FLAG_HIDDEN);
            packet.tx.testmotor_toggle = !lv_obj_has_flag(ui_TestMotorLeverRunButton, LV_OBJ_FLAG_HIDDEN);
            if (lv_obj_has_state(ui_TestMotorMeterButton, LV_STATE_USER_1))
            {
                packet.tx.testmotor_meter_type = 0;
            }
            else if (lv_obj_has_state(ui_TestMotorMeterButton, LV_STATE_USER_2))
            {
                packet.tx.testmotor_meter_type = 1;
            }
            else // if (lv_obj_has_state(ui_TestMotorMeterButton, LV_STATE_USER_3))
            {
                packet.tx.testmotor_meter_type = 2;
            }

            // UI画面がAction可能状態かつSTMからの要求がある場合のみ実行許可
            if (isActionState(ui_state) && packet.rx.action_run)
            {
                packet.tx.action_run = true;
                action_run = true;
            }
            else
            {
                packet.tx.action_run = false;
                action_run = false;
            }
        }

        // フラグ変化判定
        just_action_run_started = (action_run && !last_action_run);
        just_action_run_stopped = (!action_run && last_action_run);
        last_action_run = action_run;

        // モード切り替え時にバッファをクリアして通信を安定化
        if (just_action_run_started)
        {
            action_run_packet.reset();
        }
        else if (just_action_run_stopped)
        {
            packet.reset();
        }

        last_time = millis();
    }

    // モニター / UI描画
    if (action_run)
    {
        if (just_action_run_started)
        {
            M5.Display.clear(TFT_BLACK);

            M5.Display.setTextSize(2);
            M5.Display.setTextColor(TFT_RED, TFT_BLACK);
            if (ui_state == ACTION_OFFENSE)
            {
                M5.Display.drawCentreString("OFFENSE IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            }
            else if (ui_state == ACTION_DEFENSE)
            {
                M5.Display.drawCentreString("DEFENCE IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            }
            else if (ui_state == ACTION_RADIOCONTROL)
            {
                M5.Display.drawCentreString("RADIOCONTROL IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            }
            else
            {
                M5.Display.drawCentreString("UNKNOWN", screen_width / 2, screen_height / 2 - 16);
            }

            M5.Display.setTextSize(2);
            if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_1))
            {
                M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
                M5.Display.drawCentreString("YELLOW GOAL", screen_width / 2, screen_height / 2 + 8);
            }
            else if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_2))
            {
                M5.Display.setTextColor(TFT_SKYBLUE, TFT_BLACK);
                M5.Display.drawCentreString("BLUE GOAL", screen_width / 2, screen_height / 2 + 8);
            }
            else // LV_STATE_USER_3
            {
                M5.Display.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);
                M5.Display.drawCentreString("GYRO", screen_width / 2, screen_height / 2 + 8);
            }
        }
    }
    else
    {
        static uint32_t last_ms = millis();
        uint32_t cur_ms = millis();
        uint32_t elapsed = just_action_run_stopped ? 0 : (cur_ms - last_ms);
        lv_tick_inc(elapsed);
        last_ms = cur_ms;

        if (just_action_run_stopped)
        {
            lv_obj_invalidate(lv_scr_act()); // 画面全体を再描画対象にする
        }

        M5.update();

        switch (ui_state)
        {
        case HOME:
            break;
        case ACTION_OFFENSE:
        case ACTION_DEFENSE:
        case ACTION_RADIOCONTROL:
            my_lv_set_object_rotation(ui_ActionMeterPointorPanel, 0, 43);
            break;
        case TEST_KICKER:
            break;
        case TEST_DRIBBLER:
            break;
        case TEST_MOTOR:
            my_lv_set_object_rotation(ui_TestMotorMeterPointorPanel, 0, 43);
            break;
        case SENSORMONITOR_BALL:
            my_lv_set_object_rotation(ui_SensorMonitorBallMeterPointorPanelA, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorBallMeterPointorPanelB, 0, 43);
            break;
        case SENSORMONITOR_LINE:
            my_lv_set_object_rotation(ui_SensorMonitorLineMeterPointorPanelA, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorLineMeterPointorPanelB, 0, 43);
            break;
        case SENSORMONITOR_GYRO:
            my_lv_set_object_rotation(ui_SensorMonitorGyroMeterPointorPanelA, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorGyroMeterPointorPanelB, 0, 43);
            break;
        case SENSORMONITOR_GOAL:
            my_lv_set_object_rotation(ui_SensorMonitorGoalMeterPointorPanelA, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorGoalMeterPointorPanelB, 0, 43);
            break;
        case SENSORMONITOR_LIDAR:
            break;
        case COMMUNICATION_TRANSMIT:
            break;
        case COMMUNICATION_RECEIVE:
            break;
        }

        lv_timer_handler();
        yield();
    }
}