#pragma once

#include <Arduino.h>
#include "Timer.hpp"

class Button
{
private:
    uint8_t _pin;
    uint8_t _pinmode;

    bool _pressed = false;         // 現在の安定した状態
    bool _oldpressed = false;      // 前回の安定した状態
    bool _pressed_moment = false;  // 押された瞬間フラグ
    bool _released_moment = false; // 離された瞬間フラグ

    bool _last_raw_state = false; // 前回の生のピン状態
    unsigned long _pushing_time = 0;

    Timer _debounceTimer; // チャタリング防止用
    Timer _pushingTimer;  // 押し続け時間計測用

    const unsigned long DEBOUNCE_DELAY = 5; // チャタリング防止時間(ms)

public:
    Button() = default;

    // 初期化: ピン番号とモード(INPUT_PULLUP / INPUT_PULLDOWN)
    bool init(uint8_t pin, uint8_t pinmode);

    // 状態更新: 毎ループ呼び出す
    void update();

    // ゲッター
    bool isPushing();            // 押されている間 true
    bool isPressedMoment();      // 押された瞬間だけ true
    bool isReleasedMoment();     // 離された瞬間だけ true
    unsigned long pushingTime(); // 押されている時間をミリ秒で返す

    void pushingTimeReset(); // 押し続け時間をリセット
};