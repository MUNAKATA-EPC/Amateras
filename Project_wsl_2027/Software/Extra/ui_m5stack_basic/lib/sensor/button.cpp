#include "button.hpp"

bool Button::init(uint8_t pin, uint8_t pinmode)
{
    _pin = pin;
    _pinmode = pinmode;
    pinMode(_pin, _pinmode);

    // 初期の生の状態を取得しておく
    if (_pinmode == INPUT_PULLDOWN)
    {
        _last_raw_state = (digitalRead(_pin) == HIGH);
    }
    else
    {
        _last_raw_state = (digitalRead(_pin) == LOW);
    }
    _pressed = _last_raw_state;
    _oldpressed = _pressed;

    return true;
}

void Button::update()
{
    // 現在の生のピン状態を取得 (アクティブ時に true)
    bool current_state = false;
    if (_pinmode == INPUT_PULLDOWN)
    {
        current_state = (digitalRead(_pin) == HIGH);
    }
    else if (_pinmode == INPUT_PULLUP)
    {
        current_state = (digitalRead(_pin) == LOW);
    }

    // チャタリング防止処理
    if (current_state != _last_raw_state)
    {
        _debounceTimer.reset();
    }

    if (_debounceTimer.msTime() > DEBOUNCE_DELAY)
    {
        if (current_state != _pressed)
        {
            _pressed = current_state;
        }
    }
    _last_raw_state = current_state;

    // 瞬間移動の検出
    _pressed_moment = (_pressed && !_oldpressed);
    _released_moment = (!_pressed && _oldpressed);

    // 押し続け時間の計測
    if (_pressed)
    {
        if (_pressed_moment)
        {
            _pushingTimer.reset(); // 押された瞬間に計測開始
        }
        _pushing_time = _pushingTimer.msTime();
    }
    else
    {
        _pushing_time = 0;
    }

    // 次回判定用に現在の確定状態を保存
    _oldpressed = _pressed;
}

bool Button::isPushing()
{
    return _pressed;
}

bool Button::isPressedMoment()
{
    return _pressed_moment;
}

bool Button::isReleasedMoment()
{
    return _released_moment;
}

unsigned long Button::pushingTime()
{
    return _pushing_time;
}

void Button::pushingTimeReset()
{
    _pushing_time = 0;
    _pushingTimer.reset();
}