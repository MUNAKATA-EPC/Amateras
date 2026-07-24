#include <lvgl.h>
#include <M5Unified.h>
#include "ui.h"

void my_lv_set_object_rotation(lv_obj_t *target, float angle_deg, float radius = 0)
{
    if (target == NULL)
        return;

    lv_obj_t *parent = lv_obj_get_parent(target);
    if (parent == NULL)
        return;

    if (radius == 0)
    {
        int16_t initial_y = lv_obj_get_y_aligned(target);
        radius = abs(initial_y);
    }

    float rad = radians(angle_deg);

    int32_t offset_x = (int32_t)(radius * sinf(rad));
    int32_t offset_y = (int32_t)(-radius * cosf(rad));

    lv_obj_set_align(target, LV_ALIGN_CENTER);
    lv_obj_set_pos(target, offset_x, offset_y);
}

uint16_t my_lv_color_rgb888_to_rgb565(uint32_t c)
{
    uint16_t r = uint16_t((c >> 16) & 0xFF);
    uint16_t g = uint16_t((c >> 8) & 0xFF);
    uint16_t b = uint16_t(c & 0xFF);
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

const char *my_lv_num_dec10_to_bin32(uint32_t val)
{
    static char buf[33]; // 32桁 + 終端文字 '\0'
    for (int i = 31; i >= 0; i--)
    {
        buf[31 - i] = (val & (1UL << i)) ? '1' : '0';
    }
    buf[32] = '\0';
    return buf;
}