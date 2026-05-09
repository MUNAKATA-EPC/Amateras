#include <lvgl.h>
#include <M5Unified.h>
#include "ui.h"

static const uint32_t screenWidth  = 320;
static const uint32_t screenHeight = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 40];

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.pushImage(area->x1, area->y1, w, h, (uint16_t *)color_p);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    auto count = M5.Touch.getCount();
    if (count > 0) {
        auto t = M5.Touch.getDetail(0);
        data->state = LV_INDEV_STATE_PR;
        data->point.x = t.x;
        data->point.y = t.y;

        Serial.printf("Touch: x=%d, y=%d\n", t.x, t.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 40);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
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

void loop() {
    lv_tick_inc(5); 
    lv_timer_handler(); 

    M5.update(); 

    delay(5);
}