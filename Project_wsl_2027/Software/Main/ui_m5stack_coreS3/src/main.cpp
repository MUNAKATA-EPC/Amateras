#include <lvgl.h>
#include <M5Unified.h>
#include "ui.h"

static const uint32_t screenWidth  = 320;
static const uint32_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 60]; 

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

void setup()
{
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 60);

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

void loop()
{
    static uint32_t last_ms = 0;
    uint32_t cur_ms = millis();
    uint32_t elapsed = cur_ms - last_ms;
    lv_tick_inc(elapsed);
    last_ms = cur_ms;
    M5.update();
    lv_timer_handler();

    yield(); 
}