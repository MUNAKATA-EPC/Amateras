#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "motor.hpp"

// モーター設定
Motor motors[4];

// Core 0
void setup()
{
  // モーターピン設定
  motors[0].setPin(2, 4, 3);
  motors[1].setPin(5, 7, 6);
  motors[2].setPin(8, 14, 15);
  motors[3].setPin(26, 27, 28);

  Serial.begin(9600);    // PC用
  Serial1.begin(115200); // Teensy用
  Serial1.setTimeout(10);

  Serial1.flush(); // 受信バッファをクリア
  while (!Serial1.available())
    ;                     // Teensyからの信号を待つ
  rp2040.fifo.push(true); // Core1起動
}

void loop()
{
  String receive_data = "";     // 受信データの例："1F1002B1003F0014F043"
  bool is_read_success = false; // 受信に成功したかどうか

  if (Serial1.available() >= 20)
  {
    receive_data = Serial1.readStringUntil('\n');
    receive_data.trim(); // 改行コード削除

    if (receive_data.length() == 20)
    {
      is_read_success = true;
    }

    Serial.print(receive_data); // デバッグ用
  }

  if (is_read_success)
  {
    static char old_rotate[4] = {'B', 'B', 'B', 'B'}; // Bはブレーキ、Fは前進、Rは後退

    if (receive_data == "1R0002R0003R0004R000")
    {
      // 全モータ停止
      for (int i = 0; i < 4; i++)
      {
        motors[i].stop();

        old_rotate[i] = 'B'; // 記録
      }

      rp2040.fifo.push(0xFF); // モーター停止信号を送る
    }
    else
    {
      String aa = "123";

      int power[4];
      power[0] = (String(receive_data[2]) + String(receive_data[3]) + String(receive_data[4])).toInt();
      power[1] = (String(receive_data[7]) + String(receive_data[8]) + String(receive_data[9])).toInt();
      power[2] = (String(receive_data[12]) + String(receive_data[13]) + String(receive_data[14])).toInt();
      power[3] = (String(receive_data[17]) + String(receive_data[18]) + String(receive_data[19])).toInt();

      int x = power[0] - power[1];                                        // 左右の差分
      int y = power[2] - power[3];                                        // 前後の差分
      uint32_t deg = uint32_t(int(atan2(y, x) * 180 / PI) + 360) % 360UL; // 角度計算
      rp2040.fifo.push(deg);                                              // 角度を送る

      char rotate[4];
      rotate[0] = receive_data[1];
      rotate[1] = receive_data[6];
      rotate[2] = receive_data[11];
      rotate[3] = receive_data[16];

      for (int i = 0; i < 4; i++)
      {
        // 方向が変わった時の保護
        if (rotate[i] != old_rotate[i])
        {
          motors[i].stop();
          delayMicroseconds(50);
        }
        old_rotate[i] = rotate[i]; // 記録

        // モーター制御
        if (rotate[i] == 'F')
        {
          motors[i].move(power[i]);
        }
        else if (rotate[i] == 'R')
        {
          motors[i].move(-power[i]);
        }
        else
        {
          motors[i].stop();
        }
      }
    }
  }
}

// NeoPixel設定
#define PIN_NEOPIXEL 16
#define NUM_PIXELS 1
Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Core 1
void setup1()
{
  pixels.begin();
  pixels.setBrightness(30);

  // 赤で待機（Core 0からの信号待ち）
  pixels.setPixelColor(0, pixels.Color(50, 0, 0));
  pixels.show();

  while (!rp2040.fifo.available())
    ;
  rp2040.fifo.pop(); // 起動信号を消費

  // 起動イルミネーション（虹色を1周させる）
  for (uint32_t i = 0; i < 65536; i += 256)
  {
    uint32_t rgb_color = pixels.ColorHSV(i, 255, 50);
    pixels.setPixelColor(0, rgb_color);
    pixels.show();
    delay(10);
  }

  // 終了後に一旦消灯
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
  delay(100);
}

void loop1()
{
  static uint32_t current_color = pixels.Color(0, 0, 0);

  if (rp2040.fifo.available())
  {
    int deg = (int)rp2040.fifo.pop(); // 角度を受信

    if (deg == 0xFF)
    {
      current_color = pixels.Color(17, 17, 17); // 停止時は暗い白
    }
    else
    {
      // Core 0からの角度(-180~180)を0~65535に変換
      uint16_t hue = (uint16_t)(deg * 65535 / 360);
      current_color = pixels.ColorHSV(hue, 255, 50);
    }
  }

  pixels.setPixelColor(0, current_color);
  pixels.show();
  delay(20);
}