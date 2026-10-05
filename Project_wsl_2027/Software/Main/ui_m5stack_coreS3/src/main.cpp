#include <lvgl.h>
#include <M5Unified.h>
#include "ui.h"
#include "my_lv.h"
#include "common/timer.hpp"
#include "common/serial_packet.hpp"
#include "common/vector.hpp"

// スクリーン幅
static const uint32_t screen_width = 320;
static const uint32_t screen_height = 240;
// スクリーンバッファ
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screen_width * 60];

// 通常の送受信データ内容
struct t_data
{
    bool action_run = false;
    int8_t action_meter_type = 0;
    UI_STATE cur_state = HOME;
    bool testkicker_btn = false;
    bool testkicker_front = false;
    bool testdribbler_toggle = false;
    bool testdribbler_front = false;
    bool testmotor_toggle = false;
    int8_t testmotor_meter_type = 0;
} __attribute__((packed));
struct r_data
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
serial_packet<t_data, r_data> packet; // 通常時の送受信パケット

// アクションが起動中かどうか
bool action_run = false;
bool last_action_run = false;
bool just_action_run_started = false;
bool just_action_run_stopped = false;

// ── 変化検出用キャッシュ ──────────────────────────────
// 前回のrx値を保持し、変化があった時だけLVGLを更新する
static r_data last_rx = {};
static bool rx_changed = false; // 今ループでrxに変化があったか

// ── ヘルパー：変化があった時だけラベル更新 ───────────────
// テキスト内容が変わっていなければ lv_label_set_text_fmt を呼ばない
// → LVGLの再描画フラグが立たず、描画コストゼロ
#define UPDATE_LABEL_IF_CHANGED(label, fmt, ...)            \
    do                                                      \
    {                                                       \
        if (rx_changed)                                     \
        {                                                   \
            lv_label_set_text_fmt(label, fmt, __VA_ARGS__); \
        }                                                   \
    } while (0)

// ── ヘルパー：変化があった時だけ回転更新 ─────────────────
#define UPDATE_ROTATION_IF_CHANGED(panel, deg, r)     \
    do                                                \
    {                                                 \
        if (rx_changed)                               \
        {                                             \
            my_lv_set_object_rotation(panel, deg, r); \
        }                                             \
    } while (0)

// アクションが選択中かどうか
bool isActionState(UI_STATE state)
{
    return (state == ACTION_OFFENCE || state == ACTION_DEFENCE || state == ACTION_RADIOCONTROL);
}

hw_timer_t *lvgl_timer = NULL;

void IRAM_ATTR lvgl_tick_isr()
{
    lv_tick_inc(1); // 1ms ごとに呼ぶ
}

void setup()
{
    Serial.begin(115200);

    Serial0.begin(115200);
    packet.begin(Serial0);

    auto cfg = M5.config();
    M5.begin(cfg);

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screen_width * 60);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screen_width;
    disp_drv.ver_res = screen_height;
    disp_drv.flush_cb = my_lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_lv_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    lvgl_timer = timerBegin(0, 80, true); // 80MHz / 80 = 1MHz
    timerAttachInterrupt(lvgl_timer, &lvgl_tick_isr, true);
    timerAlarmWrite(lvgl_timer, 1000, true); // 1000us = 1ms
    timerAlarmEnable(lvgl_timer);
}

void loop()
{
    // STMとのシリアル通信
    bool newly_received = packet.update();

    if (newly_received)
    {
        rx_changed = (memcmp(&packet.rx, &last_rx, sizeof(r_data)) != 0);
        if (rx_changed)
        {
            last_rx = packet.rx;
        }
    }
    else
    {
        rx_changed = false;
    }

    // t_data 初期化
    packet.tx.action_meter_type = -1;
    packet.tx.testmotor_meter_type = -1;
    packet.tx.testkicker_btn = false;
    packet.tx.testkicker_front = false;
    packet.tx.testdribbler_toggle = false;
    packet.tx.testdribbler_front = false;
    packet.tx.testmotor_toggle = false;

    // t_data 代入
    packet.tx.cur_state = ui_state;

    // Action画面でのメータータイプ設定
    if (isActionState(ui_state))
    {
        if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_1))
            packet.tx.action_meter_type = 0;
        else if (lv_obj_has_state(ui_ActionMeterButton, LV_STATE_USER_2))
            packet.tx.action_meter_type = 1;
        else
            packet.tx.action_meter_type = 2;
    }

    if (ui_state == TEST_KICKER)
    {
        packet.tx.testkicker_btn = lv_obj_has_state(ui_TestKickerKickButton, LV_STATE_PRESSED);
        packet.tx.testkicker_front = !lv_obj_has_flag(ui_TestKickerLeverFrontButton, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_state == TEST_DRIBBLER)
    {
        packet.tx.testdribbler_toggle = !lv_obj_has_flag(ui_TestDribblerLeverRunButton, LV_OBJ_FLAG_HIDDEN);
        packet.tx.testdribbler_front = !lv_obj_has_flag(ui_TestDribblerLeverFrontButton, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_state == TEST_MOTOR)
    {
        packet.tx.testmotor_toggle = !lv_obj_has_flag(ui_TestMotorLeverRunButton, LV_OBJ_FLAG_HIDDEN);
        if (lv_obj_has_state(ui_TestMotorMeterButton, LV_STATE_USER_1))
            packet.tx.testmotor_meter_type = 0;
        else if (lv_obj_has_state(ui_TestMotorMeterButton, LV_STATE_USER_2))
            packet.tx.testmotor_meter_type = 1;
        else
            packet.tx.testmotor_meter_type = 2;
    }

    // action_run フラグ処理
    action_run = packet.rx.action_run;

    just_action_run_started = (action_run && !last_action_run);
    just_action_run_stopped = (!action_run && last_action_run);
    last_action_run = action_run;

    // 描画処理
    if (action_run)
    {
        // アクション実行中：起動直後のみ1回だけ描画
        if (just_action_run_started)
        {
            M5.Display.clear(TFT_BLACK);

            M5.Display.setTextSize(2);
            M5.Display.setTextColor(TFT_RED, TFT_BLACK);
            if (ui_state == ACTION_OFFENCE)
                M5.Display.drawCentreString("OFFENSE IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            else if (ui_state == ACTION_DEFENCE)
                M5.Display.drawCentreString("DEFENCE IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            else if (ui_state == ACTION_RADIOCONTROL)
                M5.Display.drawCentreString("RADIOCONTROL IS RUNNING", screen_width / 2, screen_height / 2 - 16);
            else
                M5.Display.drawCentreString("UNKNOWN", screen_width / 2, screen_height / 2 - 16);

            M5.Display.setTextSize(2);
            if (packet.tx.action_meter_type == 0)
            {
                M5.Display.setTextColor(my_lv_color_rgb888_to_rgb565(0xE8E839), TFT_BLACK);
                M5.Display.drawCentreString("YELLOW GOAL", screen_width / 2, screen_height / 2 + 8);
            }
            else if (packet.tx.action_meter_type == 1)
            {
                M5.Display.setTextColor(my_lv_color_rgb888_to_rgb565(0x2095F6), TFT_BLACK);
                M5.Display.drawCentreString("BLUE GOAL", screen_width / 2, screen_height / 2 + 8);
            }
            else
            {
                M5.Display.setTextColor(my_lv_color_rgb888_to_rgb565(0x525552), TFT_BLACK);
                M5.Display.drawCentreString("GYRO", screen_width / 2, screen_height / 2 + 8);
            }
        }
    }
    else
    {
        // アクション停止直後：全画面再描画
        if (just_action_run_stopped)
        {
            lv_obj_invalidate(lv_scr_act());
            rx_changed = true; // 強制的に全ラベル・回転を更新
        }

        M5.update();

        // 各画面のUI更新：rx変化時のみ実行
        switch (ui_state)
        {
        case HOME:
            break;

        case ACTION_OFFENCE:
        case ACTION_RADIOCONTROL:
        case ACTION_DEFENCE:
            UPDATE_LABEL_IF_CHANGED(ui_ActionDebugLabel,
                                    "ball_deg: %d\nball_dis: %d\ngyro_deg: %d\nyellow_goal_deg: %d\nyellow_goal_dis: %d\nblue_goal_deg: %d\nblue_goal_dis: %d",
                                    packet.rx.ball_deg, packet.rx.ball_dis, packet.rx.gyro_deg,
                                    packet.rx.yellow_goal_deg, packet.rx.yellow_goal_dis,
                                    packet.rx.blue_goal_deg, packet.rx.blue_goal_dis);

            if (packet.tx.action_meter_type == 0)
                UPDATE_ROTATION_IF_CHANGED(ui_ActionMeterPointorPanel, packet.rx.yellow_goal_deg, 43);
            else if (packet.tx.action_meter_type == 1)
                UPDATE_ROTATION_IF_CHANGED(ui_ActionMeterPointorPanel, packet.rx.blue_goal_deg, 43);
            else
                UPDATE_ROTATION_IF_CHANGED(ui_ActionMeterPointorPanel, packet.rx.gyro_deg, 43);
            break;

        case TEST_KICKER:
            break;

        case TEST_DRIBBLER:
            break;

        case TEST_MOTOR:
            UPDATE_LABEL_IF_CHANGED(ui_TestMotorDebugLabel,
                                    "ball_deg: %d\nball_dis: %d\ngyro_deg: %d\nyellow_goal_deg: %d\nyellow_goal_dis: %d\nblue_goal_deg: %d\nblue_goal_dis: %d",
                                    packet.rx.ball_deg, packet.rx.ball_dis, packet.rx.gyro_deg,
                                    packet.rx.yellow_goal_deg, packet.rx.yellow_goal_dis,
                                    packet.rx.blue_goal_deg, packet.rx.blue_goal_dis);

            if (packet.tx.testmotor_meter_type == 0)
                UPDATE_ROTATION_IF_CHANGED(ui_TestMotorMeterPointorPanel, packet.rx.yellow_goal_deg, 43);
            else if (packet.tx.testmotor_meter_type == 1)
                UPDATE_ROTATION_IF_CHANGED(ui_TestMotorMeterPointorPanel, packet.rx.blue_goal_deg, 43);
            else
                UPDATE_ROTATION_IF_CHANGED(ui_TestMotorMeterPointorPanel, packet.rx.gyro_deg, 43);
            break;

        case SENSORMONITOR_BALL:
            UPDATE_LABEL_IF_CHANGED(ui_SensorMonitorBallDebugLabel,
                                    "ball_deg: %d\nball_dis: %d",
                                    packet.rx.ball_deg, packet.rx.ball_dis);
            UPDATE_ROTATION_IF_CHANGED(ui_SensorMonitorBallMeterPointorPanelA, packet.rx.ball_deg, 43);
            break;

        case SENSORMONITOR_LINE:
            UPDATE_LABEL_IF_CHANGED(ui_SensorMonitorLineDebugLabel,
                                    "line_angel: %s\nline_right_side: %d\nline_left_side: %d",
                                    my_lv_num_dec10_to_bin32(packet.rx.line_angel),
                                    packet.rx.line_right_side_val, packet.rx.line_left_side_val);
            break;

        case SENSORMONITOR_GYRO:
            UPDATE_LABEL_IF_CHANGED(ui_SensorMonitorGyroDebugLabel,
                                    "gyro_deg: %d",
                                    packet.rx.gyro_deg);
            UPDATE_ROTATION_IF_CHANGED(ui_SensorMonitorGyroMeterPointorPanelA, packet.rx.gyro_deg, 43);
            break;

        case SENSORMONITOR_GOAL:
            UPDATE_LABEL_IF_CHANGED(ui_SensorMonitorGoalDebugLabel,
                                    "yellow_goal_deg: %d\nyellow_goal_dis: %d\nblue_goal_deg: %d\nblue_goal_dis: %d",
                                    packet.rx.yellow_goal_deg, packet.rx.yellow_goal_dis,
                                    packet.rx.blue_goal_deg, packet.rx.blue_goal_dis);
            UPDATE_ROTATION_IF_CHANGED(ui_SensorMonitorGoalMeterPointorPanelA, packet.rx.yellow_goal_deg, 43);
            UPDATE_ROTATION_IF_CHANGED(ui_SensorMonitorGoalMeterPointorPanelB, packet.rx.blue_goal_deg, 43);
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
