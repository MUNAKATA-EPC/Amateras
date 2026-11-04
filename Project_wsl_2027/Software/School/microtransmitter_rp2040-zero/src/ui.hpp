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

    /**
     * @brief 1つのボタンを保持し、画面に描画するクラス
     *
     * x, y は外枠の左上、w, h は外枠の幅と高さです
     * ボタン面は外枠から2px内側に描画します
     */
    class button
    {
    public:
        /**
         * @brief 空欄用のボタンを作る
         */
        button()
            : _x(0), _y(0), _w(0), _h(0),
              _text(""), _valid(false)
        {
        }

        /**
         * @brief 指定位置とサイズでボタンを作る
         *
         * @param x     外枠の左上x座標
         * @param y     外枠の左上y座標
         * @param w     外枠の幅
         * @param h     外枠の高さ
         * @param text  ボタンに表示するUTF-8文字列
         */
        button(int x, int y, int w, int h, const char *text)
            : _x(x), _y(y), _w(w), _h(h),
              _text(text != nullptr ? text : ""),
              _valid(x >= 0 && y >= 0 && w > 4 && h > 4)
        {
        }

        /**
         * @brief このボタンが空欄か調べる
         *
         * @return 空欄ならtrue
         */
        bool is_empty() const
        {
            return !_valid;
        }

        /**
         * @brief ボタンの表示文字列を取得する
         *
         * @return ボタンの文字列で空欄なら空文字列
         */
        const char *get_text() const
        {
            return _text;
        }

        /**
         * @brief ボタンを現在の状態で描画する
         *
         * @param over 選択中ならtrueで外枠を描く
         * @param push 押下中ならtrueで白い外側と黒いボタン面を描く
         */
        void process(bool over, bool push)
        {
            if (!_valid)
                return;

            const uint8_t previous_draw_color = display.getDrawColor();

            const uint16_t face_x = (uint16_t)(_x + 2);
            const uint16_t face_y = (uint16_t)(_y + 2);
            const uint16_t face_w = (uint16_t)(_w - 4);
            const uint16_t face_h = (uint16_t)(_h - 4);

            // 前回の枠や押下表示を消してから描き直す
            display.setDrawColor(0);
            display.drawBox(
                (uint16_t)_x,
                (uint16_t)_y,
                (uint16_t)_w,
                (uint16_t)_h);

            if (push)
            {
                // 押下中：外側を白、ボタン面を黒にする
                display.setDrawColor(1);
                display.drawBox(
                    (uint16_t)_x,
                    (uint16_t)_y,
                    (uint16_t)_w,
                    (uint16_t)_h);

                display.setDrawColor(0);
                display.drawBox(face_x, face_y, face_w, face_h);
            }
            else
            {
                // 通常：ボタン面を白にする
                display.setDrawColor(1);
                display.drawBox(face_x, face_y, face_w, face_h);

                // 選択中：外枠を黒で描く
                if (over)
                {
                    display.setDrawColor(0);
                    display.drawFrame(
                        (uint16_t)_x,
                        (uint16_t)_y,
                        (uint16_t)_w,
                        (uint16_t)_h);
                }
            }

            const int32_t ascent = display.getAscent();
            const int32_t descent = display.getDescent();
            const int32_t font_height = ascent - descent;
            const int32_t text_width = display.getUTF8Width(_text);

            int32_t text_x =
                (int32_t)face_x + ((int32_t)face_w - text_width) / 2;

            // 文字がボタン面より幅広い場合は左端に合わせる
            if (text_x < (int32_t)face_x)
                text_x = face_x;

            const int32_t baseline_y =
                (int32_t)face_y +
                ((int32_t)face_h - font_height) / 2 +
                ascent;

            // U8g2にはフォントモードの取得関数がないため、透過モードに設定する
            // モードは以降も透過のままになる。
            display.setFontMode(1);
            display.setDrawColor(push ? 1 : 0);
            display.setCursor((uint16_t)text_x, (uint16_t)baseline_y);
            display.print(_text);

            display.setDrawColor(previous_draw_color);
        }

    private:
        int16_t _x;
        int16_t _y;
        int16_t _w;
        int16_t _h;
        const char *_text;
        bool _valid;
    };

    /**
     * @brief 任意サイズの2次元ボタン配列を管理するクラス
     *
     * 配列を作った後、その配列をbutton_containerに渡します
     * 配列はbutton_containerより長く存続させてください
     */
    class button_container
    {
    private:
        /**
         * @brief 2次元配列から指定位置のボタンを取得する
         *
         * @tparam Rows 配列の行数
         * @tparam Cols 配列の列数
         * @param data  2次元配列の先頭アドレス
         * @param row   取得する行
         * @param col   取得する列
         * @return 指定位置のボタンへの参照
         */
        template <size_t Rows, size_t Cols>
        static button &_array_at(void *data, size_t row, size_t col)
        {
            button(*array)[Rows][Cols] =
                static_cast<button(*)[Rows][Cols]>(data);

            return (*array)[row][col];
        }

        /**
         * @brief 指定位置のボタンを取得する
         *
         * @param row 行
         * @param col 列
         * @return 指定位置のボタンへの参照
         */
        button &_at(size_t row, size_t col)
        {
            return _accessor(_data, row, col);
        }

        /**
         * @brief 最初に見つかった空欄でないボタンを選択する
         */
        void _find_first_button()
        {
            _has_selection = false;

            for (size_t row = 0; row < _rows; row++)
            {
                for (size_t col = 0; col < _cols; col++)
                {
                    if (!_at(row, col).is_empty())
                    {
                        _selected_row = row;
                        _selected_col = col;
                        _has_selection = true;
                        return;
                    }
                }
            }
        }

        /**
         * @brief 同じ行で左右に移動する
         *
         * 空欄を飛ばし、移動先がなければ現在位置にとどまります
         *
         * @param direction 右なら1、左なら-1
         */
        void _move_horizontal(int8_t direction)
        {
            int32_t col = (int32_t)_selected_col + direction;

            while (col >= 0 && col < (int32_t)_cols)
            {
                if (!_at(_selected_row, (size_t)col).is_empty())
                {
                    _selected_col = (size_t)col;
                    return;
                }

                col += direction;
            }
        }

        /**
         * @brief 上下方向に移動する
         *
         * 空欄だけの行を飛ばし移動先の行では、現在の列に最も近い
         * ボタンを選べ、同距離なら左側の列を選びます
         *
         * @param direction 上なら-1、下なら1
         */
        void _move_vertical(int8_t direction)
        {
            int32_t row = (int32_t)_selected_row + direction;

            while (row >= 0 && row < (int32_t)_rows)
            {
                int32_t nearest_col = -1;
                int32_t nearest_distance = 2147483647;

                for (size_t col = 0; col < _cols; col++)
                {
                    if (_at((size_t)row, col).is_empty())
                        continue;

                    int32_t distance = (int32_t)col - (int32_t)_selected_col;
                    if (distance < 0)
                        distance = -distance;

                    if (distance < nearest_distance)
                    {
                        nearest_distance = distance;
                        nearest_col = (int32_t)col;
                    }
                }

                if (nearest_col >= 0)
                {
                    _selected_row = (size_t)row;
                    _selected_col = (size_t)nearest_col;
                    return;
                }

                row += direction;
            }
        }

    public:
        /**
         * @brief 2次元ボタン配列を管理するコンテナを作る
         *
         * 行数と列数は配列の宣言から自動取得します
         *
         * @tparam Rows 配列の行数
         * @tparam Cols 配列の列数
         * @param buttons 管理する2次元ボタン配列
         */
        template <size_t Rows, size_t Cols>
        explicit button_container(button (&buttons)[Rows][Cols])
            : _data(static_cast<void *>(&buttons)),
              _rows(Rows),
              _cols(Cols),
              _accessor(&_array_at<Rows, Cols>),
              _selected_row(0),
              _selected_col(0),
              _has_selection(false),
              _previous_right(false),
              _previous_left(false),
              _previous_front(false),
              _previous_rear(false),
              _previous_confirm(false)
        {
            _find_first_button();
        }

        /**
         * @brief 入力に応じて選択位置を更新し、全ボタンを描画する
         *
         * frontは上、rearは下です
         * 入力が押された瞬間に一度だけ移動します
         *
         * @param right   右入力
         * @param left    左入力
         * @param front   上入力
         * @param rear    下入力
         * @param confirm 決定入力
         * @return 決定入力が押された瞬間ならtrue
         */
        bool process(
            bool right,
            bool left,
            bool front,
            bool rear,
            bool confirm)
        {
            const bool right_pressed = right && !_previous_right;
            const bool left_pressed = left && !_previous_left;
            const bool front_pressed = front && !_previous_front;
            const bool rear_pressed = rear && !_previous_rear;
            const bool confirm_pressed = confirm && !_previous_confirm;

            _previous_right = right;
            _previous_left = left;
            _previous_front = front;
            _previous_rear = rear;
            _previous_confirm = confirm;

            if (_has_selection)
            {
                if (right_pressed)
                    _move_horizontal(1);
                else if (left_pressed)
                    _move_horizontal(-1);
                else if (front_pressed)
                    _move_vertical(-1);
                else if (rear_pressed)
                    _move_vertical(1);
            }

            for (size_t row = 0; row < _rows; row++)
            {
                for (size_t col = 0; col < _cols; col++)
                {
                    const bool selected =
                        _has_selection &&
                        row == _selected_row &&
                        col == _selected_col;

                    _at(row, col).process(selected, selected && confirm);
                }
            }

            return _has_selection && confirm_pressed;
        }

        /**
         * @brief 現在選択中の行番号を取得する
         *
         * @return 0始まりの行番号で選択がない場合は0
         */
        size_t get_selected_row() const
        {
            return _selected_row;
        }

        /**
         * @brief 現在選択中の列番号を取得する
         *
         * @return 0始まりの列番号で選択がない場合は0
         */
        size_t get_selected_col() const
        {
            return _selected_col;
        }

        /**
         * @brief 選択可能なボタンがあるか調べる
         *
         * @return 空欄でないボタンがあればtrue
         */
        bool has_selection() const
        {
            return _has_selection;
        }

    private:
        typedef button &(*_button_accessor)(void *, size_t, size_t);

        void *_data;
        size_t _rows;
        size_t _cols;
        _button_accessor _accessor;

        size_t _selected_row;
        size_t _selected_col;
        bool _has_selection;

        bool _previous_right;
        bool _previous_left;
        bool _previous_front;
        bool _previous_rear;
        bool _previous_confirm;
    };
}