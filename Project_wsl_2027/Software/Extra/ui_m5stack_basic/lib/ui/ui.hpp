#pragma once

#include <Arduino.h>
#include <SPIFFS.h>

#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>

// 自作ライブラリ
#include "uiMouse.hpp"
#include "uiControllObject.hpp"

bool uiInit();
void uiUpdate(mouse_t mouse);
int uiWidth();
int uiHeight();