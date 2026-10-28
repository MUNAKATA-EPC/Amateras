#pragma once

#include <stdio.h>
#include <string.h>

#include <Wire.h>
#include <U8g2lib.h>

//// ui ////
namespace ui
{
    U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C display(
        U8G2_R0,
        U8X8_PIN_NONE);

    // 幅/高さ
    const uint16_t display_width = display.getDisplayWidth();
    const uint16_t display_height = display.getDisplayHeight();

    /**
     * @brief   UTF-8文字が何バイトかを調べる関数（日本語は3バイト）
     * @param p 文字列
     * @return  pのバイト数
     */
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

    /**
     * @brief textを自動改行して描画する関数
     *
     * @param text          描画したい文字列
     * @param x             開始x座標
     * @param baseline_y    開始基準線y座標
     * @param max_width     描画最大幅
     * @param ascii_gap_px  文字同士のpx幅
     *
     * @return pのバイト数
     */
    void utf8_printwrapped(
        const char *text,
        uint16_t x,
        uint16_t baseline_y,
        uint16_t max_width,
        uint8_t ascii_gap_px = 1,
        bool white = false)
    {
        if (x >= display_width || max_width == 0)
            return;

        if (max_width > display_width - x)
            max_width = display_width - x;

        const uint16_t right_edge = x + max_width;
        const int16_t ascent = display.getAscent();
        const int16_t descent = display.getDescent();
        const uint16_t font_height = ascent - descent;
        const uint16_t line_pitch = font_height + 2;
        const uint8_t previous_draw_color = display.getDrawColor();

        // 黒文字を白い領域に描くため、文字の背景は透過にする
        if (!white)
            display.setFontMode(1);

        display.setDrawColor(!white ? 0 : 1);

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
                cursor_y += line_pitch;
                p++;
                continue;
            }

            const uint8_t bytes = utf8_charbytes(p);
            char glyph[5];

            for (uint8_t i = 0; i < bytes; i++)
                glyph[i] = (char)p[i];
            glyph[bytes] = '\0';

            const uint16_t glyph_width = display.getUTF8Width(glyph);

            const bool is_ascii_letter_or_digit =
                bytes == 1 &&
                ((glyph[0] >= 'A' && glyph[0] <= 'Z') ||
                 (glyph[0] >= 'a' && glyph[0] <= 'z') ||
                 (glyph[0] >= '0' && glyph[0] <= '9'));

            const uint8_t gap =
                is_ascii_letter_or_digit ? ascii_gap_px : 0;

            const uint16_t cell_width = glyph_width + gap;

            // 文字と後ろの間隔が右端を超える場合は改行
            if (cursor_x > x && cursor_x + cell_width > right_edge)
            {
                cursor_x = x;
                cursor_y += line_pitch;
            }

            if (!white && cell_width > 0)
            {
                // baselineからフォントの上下端を計算して白く塗る
                int16_t box_top = (int16_t)cursor_y - ascent;
                const int16_t box_bottom = (int16_t)cursor_y - descent;

                if (box_top < 0)
                    box_top = 0;

                if (box_bottom > box_top)
                {
                    display.setDrawColor(1);
                    display.drawBox(
                        cursor_x,
                        (uint16_t)box_top,
                        cell_width,
                        (uint16_t)(box_bottom - box_top));
                }

                display.setDrawColor(0);
            }

            display.setCursor(cursor_x, cursor_y);
            display.print(glyph);

            cursor_x += cell_width;
            p += bytes;
        }

        // 呼び出し元の描画色に戻す
        display.setDrawColor(previous_draw_color);
    }
}