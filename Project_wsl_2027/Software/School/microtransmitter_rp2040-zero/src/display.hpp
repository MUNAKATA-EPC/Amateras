#pragma once

#include <stdio.h>
#include <string.h>

#include <Wire.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE);

const uint16_t displaywidth = display.getDisplayWidth();
const uint16_t displayheight = display.getDisplayHeight();

// UTF-8文字が何バイトかを調べる（日本語は3バイト）
uint8_t utf8_charbytes(const uint8_t *p)
{
    if ((*p & 0x80) == 0x00)
        return 1;
    if ((*p & 0xE0) == 0xC0)
        return 2;
    if ((*p & 0xF0) == 0xE0)
        return 3;
    if ((*p & 0xF8) == 0xF0)
        return 4;
    return 1;
}

// x, baselineY を開始位置に、max_width の範囲で自動改行して描画
void utf8_printwrapped(
    const char *text,
    uint16_t x,
    uint16_t baseline_y,
    uint16_t max_width,
    uint8_t ascii_gappx = 1)
{
    if (x >= displaywidth)
        return;
    if (max_width > displaywidth - x)
    {
        max_width = displaywidth - x;
    }

    const uint16_t rightedge = x + max_width;
    const uint16_t linepitch = displayheight + 2;

    uint16_t cursor_x = x;
    uint16_t cursor_y = baseline_y;
    const uint8_t *p = (const uint8_t *)text;

    while (*p != '\0')
    {
        if (*p == '\r')
        {
            p++;
            continue;
        }
        if (*p == '\n')
        {
            cursor_x = x;
            cursor_y += linepitch;
            p++;
            continue;
        }

        uint8_t bytes = utf8_charbytes(p);
        char glyph[5];

        for (uint8_t i = 0; i < bytes; i++)
        {
            glyph[i] = (char)p[i];
        }
        glyph[bytes] = '\0';

        uint16_t glyphwidth = display.getUTF8Width(glyph);

        // 英字・数字の後ろに指定ピクセル分の間隔を足す
        bool isAsciiLetterOrDigit =
            bytes == 1 &&
            ((glyph[0] >= 'A' && glyph[0] <= 'Z') ||
             (glyph[0] >= 'a' && glyph[0] <= 'z') ||
             (glyph[0] >= '0' && glyph[0] <= '9'));

        uint8_t gap = isAsciiLetterOrDigit ? ascii_gappx : 0;

        // 次の文字と間隔が右端を超えるなら改行
        if (cursor_x > x && cursor_x + glyphwidth > rightedge)
        {
            cursor_x = x;
            cursor_y += linepitch;
        }

        display.setCursor(cursor_x, cursor_y);
        display.print(glyph);

        cursor_x += glyphwidth + gap;
        p += bytes;
    }
}