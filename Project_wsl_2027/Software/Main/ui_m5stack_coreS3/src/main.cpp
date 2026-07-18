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
    UI_STATE ui_state = HOME;
    bool test_kicker_btn = false;
    bool test_dribbler_toggle = false;
    bool test_motor_toggle = false;
};
struct r_data
{
    bool action_run = false;
};
serial_packet<t_data, r_data> packet(20);
struct action_run_t_data
{
    int peer_posi_x = 0;
    int peer_posi_y = 0;
};
struct action_run_r_data
{
    bool action_run = false;
    int my_posi_x = 0;
    int my_posi_y = 0;
};
serial_packet<action_run_t_data, action_run_r_data> action_run_packet(20);
bool action_run = false;
bool test_kicker_btn = false;
bool test_dribbler_toggle = false;
bool test_motor_toggle = false;

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
    static bool prev_action_run = false;

    if (!action_run)
    {
        if (prev_action_run)
        {
            while (Serial0.available()) Serial0.read();
            lv_obj_invalidate(lv_scr_act());
        }
        prev_action_run = false;

        static uint32_t last_ms = 0;
        uint32_t cur_ms = millis();
        uint32_t elapsed = cur_ms - last_ms;
        lv_tick_inc(elapsed);
        last_ms = cur_ms;

        M5.update();

        packet.tx.ui_state = ui_state;
        packet.update();

        action_run = (isActionState(ui_state) && packet.rx.action_run);

        switch (ui_state)
        {
        case HOME:
            break;
        case ACTION_OFFENSE:
            my_lv_set_object_rotation(ui_ActionMeterPointorPanel, 0, 43);
            break;
        case ACTION_DEFENSE:
            my_lv_set_object_rotation(ui_ActionMeterPointorPanel, 0, 43);
            break;
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
            my_lv_set_object_rotation(ui_SensorMonitorBallMeterPointorPanel1, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorBallMeterPointorPanel2, 0, 43);
            break;
        case SENSORMONITOR_LINE:
            my_lv_set_object_rotation(ui_SensorMonitorLineMeterPointorPanel1, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorLineMeterPointorPanel2, 0, 43);
            break;
        case SENSORMONITOR_GYRO:
            my_lv_set_object_rotation(ui_SensorMonitorGyroMeterPointorPanel1, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorGyroMeterPointorPanel2, 0, 43);
            break;
        case SENSORMONITOR_GOAL:
            my_lv_set_object_rotation(ui_SensorMonitorGoalMeterPointorPanel1, 0, 43);
            my_lv_set_object_rotation(ui_SensorMonitorGoalMeterPointorPanel2, 0, 43);
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
    else
    {
        if (!prev_action_run)
        {
            while (Serial0.available()) Serial0.read();
            M5.Display.clear(TFT_BLACK);
            M5.Display.setTextSize(2);
            M5.Display.setTextColor(TFT_RED, TFT_BLACK);
            M5.Display.drawCentreString("ACTION IS RUNNING", screen_width / 2, screen_height / 2 - 8);
        }
        prev_action_run = true;

        action_run_packet.update();

        action_run = action_run_packet.rx.action_run;
        test_kicker_btn = false;
        test_dribbler_toggle = false;
        test_motor_toggle = false;
    }

    static uint32_t _last_tx_time = millis();
    if (millis() - _last_tx_time >= 20)
    {
        // Serial.println(sizeof(ui_state));
    }
}