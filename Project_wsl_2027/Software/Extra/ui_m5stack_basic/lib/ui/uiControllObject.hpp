#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <LovyanGFX.hpp>
#include <cstdint>
#include "uiMouse.hpp"

//// Buttonオブジェクト ////

class uiButton
{
public:
    uiButton(int x, int y, int w, int h,
             uint32_t bg_color, const char *text, uint32_t text_color);

    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    bool isPushing() const { return _pushing; }
    bool isPressedMoment() const { return _pressed_moment; }
    bool isReleasedMoment() const { return _released_moment; }

private:
    int _x, _y, _w, _h;
    uint32_t _bg_color;
    const char *_text;
    uint32_t _text_color;
    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;
    bool _hitTest(const posi_t &posi) const;
};

class uiBitmapButton
{
public:
    uiBitmapButton(int x, int y, int w, int h,
                   const uint8_t *bitmap, int bmp_w, int bmp_h,
                   uint32_t bitmap_color, uint32_t bg_color,
                   int rotation = 0, bool mirror = false);

    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

private:
    int _x, _y, _w, _h;
    const uint8_t *_bitmap;
    int _bmp_w, _bmp_h;
    uint32_t _bitmap_color, _bg_color;

    int _rotation;
    bool _mirror;

    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;

    bool _hitTest(const posi_t &posi) const;
};

class uiImageButton
{
public:
    uiImageButton(int x, int y, int w, int h,
                  int img_w, int img_h, const char *png_path);
    ~uiImageButton();

    bool init();
    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    bool isPushing() const { return _pushing; }
    bool isPressedMoment() const { return _pressed_moment; }
    bool isReleasedMoment() const { return _released_moment; }

private:
    int _x, _y, _w, _h, _img_w, _img_h;
    const char *_png_path;
    LGFX_Sprite _sprite;
    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;
    bool _hitTest(const posi_t &posi) const;
};

class uiFlexibleImageButton
{
public:
    uiFlexibleImageButton(int x, int y, int img_x, int img_y, int w, int h,
                          uint32_t bg_color,
                          int text_xx, int text_yy, const char *text, uint32_t text_color,
                          int img_w, int img_h, const char *png_path);
    ~uiFlexibleImageButton();

    bool init();
    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    bool isPushing() const { return _pushing; }
    bool isPressedMoment() const { return _pressed_moment; }
    bool isReleasedMoment() const { return _released_moment; }

private:
    int _x, _y, _img_x, _img_y, _w, _h, _text_xx, _text_yy, _img_w, _img_h;
    uint32_t _bg_color;
    const char *_text;
    uint32_t _text_color;
    const char *_png_path;
    LGFX_Sprite _sprite;
    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;
    bool _hitTest(const posi_t &posi) const;
};

//// CycleButtonオブジェクト ////

static constexpr int ui_num_states_max = 8;

enum class uiCycleSplitMode
{
    None,
    Horizontal,
    Vertical
};

class uiCycleButton
{
public:
    uiCycleButton(int x, int y, int w, int h, int num_states,
                  const uint32_t *bg_colors, const char **texts, const uint32_t *text_colors,
                  uiCycleSplitMode split_mode = uiCycleSplitMode::None);

    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    int getIndex() const { return _current_index; }

private:
    int _x, _y, _w, _h;
    int _num_states;
    int _current_index = 0;
    bool _pushing = false;
    bool _prev_pushing = false;

    uiCycleSplitMode _split_mode;

    uint32_t _bg_colors[ui_num_states_max];
    const char *_texts[ui_num_states_max];
    uint32_t _text_colors[ui_num_states_max];

    bool _hitTest(const posi_t &posi) const;
};

class uiImageCycleButton
{
public:
    uiImageCycleButton(int x, int y, int w, int h,
                       int cycle_count,
                       int img_w, int img_h,
                       const char *const png_paths[],
                       uiCycleSplitMode split_mode = uiCycleSplitMode::None);
    ~uiImageCycleButton();

    bool init();
    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    bool isPushing() const { return _pushing; }
    bool isPressedMoment() const { return _pressed_moment; }
    bool isReleasedMoment() const { return _released_moment; }
    int currentIndex() const { return _index; }

private:
    int _x, _y, _w, _h, _img_w, _img_h;
    int _cycle_count;

    uiCycleSplitMode _split_mode;

    const char *_paths[ui_num_states_max];
    LGFX_Sprite *_sprites = nullptr;
    int _index = 0;
    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;
    bool _hitTest(const posi_t &posi) const;
};

//// Toggleオブジェクト ////

class uiToggle
{
public:
    uiToggle(int x, int y, int w, int h,
             uint32_t border_color,
             uint32_t inner_bg_color,
             uint32_t on_color, const char *on_text, uint32_t on_text_color,
             uint32_t off_color, const char *off_text, uint32_t off_text_color,
             bool initial_state = false,
             int border_w = 2);

    void update(const mouse_t &mouse);
    void draw(LGFX_Sprite &canvas);

    bool isOn() const { return _is_on; }
    bool isPushing() const { return _pushing; }
    bool isPressedMoment() const { return _pressed_moment; }
    bool isReleasedMoment() const { return _released_moment; }
    bool isToggledMoment() const { return _toggled_moment; }

private:
    int _x, _y, _w, _h;
    int _border_w;
    uint32_t _border_color, _inner_bg_color;
    uint32_t _on_color, _off_color;
    const char *_on_text, *_off_text;
    uint32_t _on_text_color, _off_text_color;
    bool _is_on = false;
    bool _pushing = false;
    bool _prev_pushing = false;
    bool _pressed_moment = false;
    bool _released_moment = false;
    bool _toggled_moment = false;

    bool _hitTest(const posi_t &posi) const;
    void _drawTextCentered(LGFX_Sprite &canvas, const char *text, int cx, int cy, uint32_t textColor);
};