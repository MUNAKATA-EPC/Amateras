#include "uiControllObject.hpp"

static void _drawCheckerOverlay(LGFX_Sprite &canvas, int x, int y, int w, int h)
{
    for (int row = 0; row < h; row++)
        for (int col = (row % 2); col < w; col += 2)
            canvas.drawPixel(x + col, y + row, TFT_BLACK);
}

//// Buttonオブジェクト ////

uiButton::uiButton(int x, int y, int w, int h,
                   uint32_t bg_color, const char *text, uint32_t text_color)
    : _x(x), _y(y), _w(w), _h(h),
      _bg_color(bg_color), _text(text), _text_color(text_color) {}

void uiButton::update(const mouse_t &mouse)
{
    _prev_pushing = _pushing;
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);
}

void uiButton::draw(LGFX_Sprite &canvas)
{
    canvas.fillRect(_x, _y, _w, _h, _bg_color);

    if (_text && _text[0] != '\0')
    {
        canvas.setTextColor(_text_color);
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.drawString(_text, _x + _w / 2, _y + _h / 2);
    }

    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

bool uiButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w && posi.y >= _y && posi.y < _y + _h);
}

uiBitmapButton::uiBitmapButton(int x, int y, int w, int h,
                               const uint8_t *bitmap, int bmp_w, int bmp_h,
                               uint32_t bitmap_color, uint32_t bg_color,
                               int rotation, bool mirror)
    : _x(x), _y(y), _w(w), _h(h),
      _bitmap(bitmap), _bmp_w(bmp_w), _bmp_h(bmp_h),
      _bitmap_color(bitmap_color), _bg_color(bg_color),
      _rotation(rotation % 4),
      _mirror(mirror)
{
}

void uiBitmapButton::update(const mouse_t &mouse)
{
    _prev_pushing = _pushing;
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);
}

void uiBitmapButton::draw(LGFX_Sprite &canvas)
{
    // 背景を塗りつぶし
    canvas.fillRect(_x, _y, _w, _h, _bg_color);

    int draw_w = (_rotation % 2 != 0) ? _bmp_h : _bmp_w;
    int draw_h = (_rotation % 2 != 0) ? _bmp_w : _bmp_h;

    int offset_x = _x + (_w - draw_w) / 2;
    int offset_y = _y + (_h - draw_h) / 2;

    if (_rotation == 0 && !_mirror)
    {
        canvas.drawBitmap(offset_x, offset_y, _bitmap, _bmp_w, _bmp_h,
                          _bitmap_color, _bg_color);
    }
    else
    {
        int bytes_per_row = (_bmp_w + 7) / 8;

        for (int y = 0; y < _bmp_h; ++y)
        {
            for (int x = 0; x < _bmp_w; ++x)
            {
                int byte_idx = y * bytes_per_row + (x / 8);
                bool is_set = _bitmap[byte_idx] & (1 << (7 - (x % 8)));

                if (is_set)
                {
                    int mx = _mirror ? (_bmp_w - 1 - x) : x;
                    int my = y;

                    int dx = mx, dy = my;
                    if (_rotation == 1)
                    {
                        dx = _bmp_h - 1 - my;
                        dy = mx;
                    }
                    else if (_rotation == 2)
                    {
                        dx = _bmp_w - 1 - mx;
                        dy = _bmp_h - 1 - my;
                    }
                    else if (_rotation == 3)
                    {
                        dx = my;
                        dy = _bmp_w - 1 - mx;
                    }

                    canvas.drawPixel(offset_x + dx, offset_y + dy, _bitmap_color);
                }
            }
        }
    }

    if (_pushing)
    {
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
    }
}

bool uiBitmapButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w &&
            posi.y >= _y && posi.y < _y + _h);
}

uiImageButton::uiImageButton(int x, int y, int w, int h,
                             int img_w, int img_h, const char *png_path)
    : _x(x), _y(y), _w(w), _h(h),
      _img_w(img_w), _img_h(img_h), _png_path(png_path) {}

uiImageButton::~uiImageButton() { _sprite.deleteSprite(); }

bool uiImageButton::init()
{
    if (!_png_path || _png_path[0] == '\0')
        return false;
    float scale_x = (float)_w / _img_w;
    float scale_y = (float)_h / _img_h;

    _sprite.setColorDepth(8);
    if (!_sprite.createSprite(_w, _h))
        return false;

    File file = SPIFFS.open(_png_path, "r");
    if (!file)
        return false;

    bool result = _sprite.drawPng(&file, 0, 0, 0, 0, 0, 0, scale_x, scale_y);
    file.close();
    return result;
}

void uiImageButton::update(const mouse_t &mouse)
{
    _prev_pushing = _pushing;
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);
}

void uiImageButton::draw(LGFX_Sprite &canvas)
{
    _sprite.pushSprite(&canvas, _x, _y);
    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

bool uiImageButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w && posi.y >= _y && posi.y < _y + _h);
}

uiFlexibleImageButton::uiFlexibleImageButton(int x, int y, int img_x, int img_y, int w, int h,
                                             uint32_t bg_color,
                                             int text_xx, int text_yy,
                                             const char *text, uint32_t text_color,
                                             int img_w, int img_h,
                                             const char *png_path)
    : _x(x), _y(y), _img_x(img_x), _img_y(img_y), _w(w), _h(h),
      _bg_color(bg_color),
      _text_xx(text_xx), _text_yy(text_yy),
      _text(text), _text_color(text_color),
      _img_w(img_w), _img_h(img_h),
      _png_path(png_path)
{
}

uiFlexibleImageButton::~uiFlexibleImageButton()
{
    _sprite.deleteSprite();
}

bool uiFlexibleImageButton::init()
{
    if (!_png_path || _png_path[0] == '\0')
        return false;

    // float scale_x = (float)_w / _img_w;
    // float scale_y = (float)_h / _img_h;

    _sprite.setColorDepth(8);
    if (!_sprite.createSprite(_w, _h))
        return false;

    // 背景色で塗りつぶしてからPNGを描画
    _sprite.fillRect(0, 0, _w, _h, _bg_color);

    File file = SPIFFS.open(_png_path, "r");
    if (!file)
        return false;

    bool result = _sprite.drawPng(&file, _img_x - _x, _img_y - _y, 0, 0, 0, 0, 1, 1); // スケーリング処理は無くす
    file.close();
    return result;
}

void uiFlexibleImageButton::update(const mouse_t &mouse)
{
    _prev_pushing = _pushing;
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);
}

void uiFlexibleImageButton::draw(LGFX_Sprite &canvas)
{
    _sprite.pushSprite(&canvas, _x, _y);

    if (_text && _text[0] != '\0')
    {
        int cx = _x + _w / 2 + _text_xx;
        int cy = _y + _h / 2 + _text_yy;
        canvas.setTextDatum(textdatum_t::middle_center);
        canvas.setTextColor(_text_color);
        canvas.drawString(_text, cx, cy);
    }

    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

bool uiFlexibleImageButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w &&
            posi.y >= _y && posi.y < _y + _h);
}

//// CycleButtonオブジェクト ////

uiCycleButton::uiCycleButton(int x, int y, int w, int h, int num_states,
                             const uint32_t *bg_colors, const char **texts, const uint32_t *text_colors,
                             uiCycleSplitMode split_mode)
    : _x(x), _y(y), _w(w), _h(h), _num_states(num_states), _split_mode(split_mode)
{
    if (_num_states > ui_num_states_max)
        _num_states = ui_num_states_max;
    for (int i = 0; i < _num_states; i++)
    {
        _bg_colors[i] = bg_colors[i];
        _texts[i] = texts[i];
        _text_colors[i] = text_colors[i];
    }
}

void uiCycleButton::update(const mouse_t &mouse)
{
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    if (_prev_pushing && !_pushing && _hitTest(mouse.posi))
    {
        int step = 1;

        if (_split_mode == uiCycleSplitMode::Horizontal)
        {
            step = (mouse.posi.x < _x + _w / 2) ? -1 : 1;
        }
        else if (_split_mode == uiCycleSplitMode::Vertical)
        {
            step = (mouse.posi.y < _y + _h / 2) ? 1 : -1;
        }

        _current_index = (_current_index + step + _num_states) % _num_states;
    }
    _prev_pushing = _pushing;
}

void uiCycleButton::draw(LGFX_Sprite &canvas)
{
    canvas.fillRect(_x, _y, _w, _h, _bg_colors[_current_index]);

    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(_text_colors[_current_index]);
    canvas.drawString(_texts[_current_index], _x + _w / 2, _y + _h / 2);

    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

bool uiCycleButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w && posi.y >= _y && posi.y < _y + _h);
}

uiImageCycleButton::uiImageCycleButton(int x, int y, int w, int h,
                                       int cycle_count,
                                       int img_w, int img_h,
                                       const char *const png_paths[],
                                       uiCycleSplitMode split_mode)
    : _x(x), _y(y), _w(w), _h(h), _img_w(img_w), _img_h(img_h),
      _cycle_count(cycle_count < ui_num_states_max ? cycle_count : ui_num_states_max),
      _split_mode(split_mode)
{
    for (int i = 0; i < _cycle_count; i++)
        _paths[i] = png_paths[i];
}

uiImageCycleButton::~uiImageCycleButton()
{
    if (_sprites)
    {
        for (int i = 0; i < _cycle_count; i++)
            _sprites[i].deleteSprite();
        delete[] _sprites;
        _sprites = nullptr;
    }
}

bool uiImageCycleButton::init()
{
    float scaleX = (float)_w / _img_w;
    float scaleY = (float)_h / _img_h;

    _sprites = new LGFX_Sprite[_cycle_count];
    if (!_sprites)
        return false;

    for (int i = 0; i < _cycle_count; i++)
    {
        _sprites[i].setColorDepth(8);
        if (!_sprites[i].createSprite(_w, _h))
            return false;

        File file = SPIFFS.open(_paths[i], "r");
        if (!file)
            return false;

        bool ok = _sprites[i].drawPng(&file, 0, 0, 0, 0, 0, 0, scaleX, scaleY);
        file.close();
        if (!ok)
            return false;
    }
    return true;
}

void uiImageCycleButton::update(const mouse_t &mouse)
{
    _prev_pushing = _pushing;
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);

    if (_pressed_moment)
    {
        int step = 1;

        if (_split_mode == uiCycleSplitMode::Horizontal)
        {
            step = (mouse.posi.x < _x + _w / 2) ? -1 : 1;
        }
        else if (_split_mode == uiCycleSplitMode::Vertical)
        {
            step = (mouse.posi.y < _y + _h / 2) ? 1 : -1;
        }

        _index = (_index + step + _cycle_count) % _cycle_count;
    }
}

void uiImageCycleButton::draw(LGFX_Sprite &canvas)
{
    if (!_sprites)
        return;
    _sprites[_index].pushSprite(&canvas, _x, _y);
    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

bool uiImageCycleButton::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w && posi.y >= _y && posi.y < _y + _h);
}

//// Toggleオブジェクト ////

uiToggle::uiToggle(int x, int y, int w, int h,
                   uint32_t border_color,
                   uint32_t inner_bg_color,
                   uint32_t on_color, const char *on_text, uint32_t on_text_color,
                   uint32_t off_color, const char *off_text, uint32_t off_text_color,
                   bool initial_state,
                   int border_w)
    : _x(x), _y(y), _w(w), _h(h), _border_w(border_w),
      _border_color(border_color), _inner_bg_color(inner_bg_color),
      _on_color(on_color), _off_color(off_color),
      _on_text(on_text), _off_text(off_text),
      _on_text_color(on_text_color), _off_text_color(off_text_color),
      _is_on(initial_state)
{
}

void uiToggle::update(const mouse_t &mouse)
{
    _pushing = _hitTest(mouse.posi) && mouse.pushing;
    _pressed_moment = (!_prev_pushing && _pushing);
    _released_moment = (_prev_pushing && !_pushing);
    _toggled_moment = false;

    if (_released_moment && _hitTest(mouse.posi))
    {
        _is_on = !_is_on;
        _toggled_moment = true;
    }
    _prev_pushing = _pushing;
}

void uiToggle::draw(LGFX_Sprite &canvas)
{
    int bw = _border_w;
    canvas.fillRect(_x, _y, _w, _h, _border_color);

    int innerX = _x + bw;
    int innerY = _y + bw;
    int innerW = _w - (bw * 2);
    int innerH = _h - (bw * 2);
    canvas.fillRect(innerX, innerY, innerW, innerH, _inner_bg_color);

    int topH = innerH / 2;
    int bottomH = innerH - topH;

    int centerX = _x + (_w / 2);
    int centerY_ON = innerY + (topH / 2);
    int centerY_OFF = innerY + topH + (bottomH / 2);

    if (_is_on)
    {
        canvas.fillRect(innerX, innerY, innerW, topH, _on_color);
        _drawTextCentered(canvas, _on_text, centerX, centerY_ON, _on_text_color);

        _drawTextCentered(canvas, _off_text, centerX, centerY_OFF, _border_color);
    }
    else
    {
        canvas.fillRect(innerX, innerY + topH, innerW, bottomH, _off_color);
        _drawTextCentered(canvas, _off_text, centerX, centerY_OFF, _off_text_color);

        _drawTextCentered(canvas, _on_text, centerX, centerY_ON, _border_color);
    }

    if (_pushing)
        _drawCheckerOverlay(canvas, _x, _y, _w, _h);
}

void uiToggle::_drawTextCentered(LGFX_Sprite &canvas, const char *text, int cx, int cy, uint32_t textColor)
{
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextColor(textColor);
    canvas.drawString(text, cx, cy);
}

bool uiToggle::_hitTest(const posi_t &posi) const
{
    return (posi.x >= _x && posi.x < _x + _w && posi.y >= _y && posi.y < _y + _h);
}