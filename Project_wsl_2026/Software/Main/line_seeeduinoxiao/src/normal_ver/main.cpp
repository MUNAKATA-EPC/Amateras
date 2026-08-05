#include <Arduino.h>
#include "multiplexer.hpp"

#include <Arduino.h>
#include "movement_average.hpp"
#include "multiplexer.hpp"
#include "button.hpp"

// 通常用の送信ヘッダー
const int start_header = 0x55; // 同期ヘッダー格納用
const int end_header = 0xAA;   // 同期ヘッダー格納用

Multiplexer line_mux;

#define LINE_SENSOR_COUNT 16
const int LINEsensor_pin[LINE_SENSOR_COUNT] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; // 前から左回りにピン番号を指定

const int LINEsensor_side_right_pin = A9; // 右サイドのピン
const int LINEsensor_side_left_pin = A8;  // 左サイドのピン
const int LINEsensor_side_back_pin = A10; // 後ろサイドのピン

const uint32_t LINEsensor_adjust_value[LINE_SENSOR_COUNT] =
    {500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500}; // センサー調整用値格納用

const uint32_t LINEsensor_side_right_adjust_value = 500UL; // 右サイドの調整値
const uint32_t LINEsensor_side_left_adjust_value = 500UL;  // 左サイドの調整値
const uint32_t LINEsensor_side_back_adjust_value = 500UL;  // 後ろサイドの調整値

const int button_pin = 0; // ボタンのピン番号
const int led_pin = 7;    // LEDのピン番号

uint32_t readMuxMedian(int pin)
{
  const int sample_count = 6;
  uint32_t v[sample_count];

  for (int i = 0; i < sample_count; i++)
    v[i] = line_mux.read(pin);

  for (int i = 0; i < sample_count - 1; i++)
    for (int j = i + 1; j < sample_count; j++)
      if (v[i] > v[j])
      {
        uint32_t t = v[i];
        v[i] = v[j];
        v[j] = t;
      }

  return v[sample_count / 2];
}

uint32_t readAnalogMedian(int pin)
{
  const int sample_count = 6;
  uint32_t v[sample_count];

  for (int i = 0; i < sample_count; i++)
    v[i] = analogRead(pin);

  for (int i = 0; i < sample_count - 1; i++)
    for (int j = i + 1; j < sample_count; j++)
      if (v[i] > v[j])
      {
        uint32_t t = v[i];
        v[i] = v[j];
        v[j] = t;
      }

  return v[sample_count / 2];
}

Button button;
void setup()
{
  Serial.begin(9600);
  Serial1.begin(115200);

  line_mux.set_pin(1, 2, 3, 4, 5, -1); // ピンを指定
  line_mux.init(10);                   // 時間を指定

  button.init(button_pin, INPUT_PULLDOWN); // ボタン初期化
  pinMode(led_pin, OUTPUT);                // LEDピン初期化
  digitalWrite(led_pin, LOW);

  pinMode(LINEsensor_side_right_pin, INPUT); // 右サイドのピン設定
  pinMode(LINEsensor_side_left_pin, INPUT);  // 左サイドのピン設定
  pinMode(LINEsensor_side_back_pin, INPUT);  // 後ろサイドのピン設定
}

void loop()
{
  static uint32_t max_line_value[19] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // エンジェルライン
  uint32_t lines_data_bit_mask = 0UL; // 初期化

  for (uint8_t i = 0; i < 16; i++)
  {
    uint32_t line_ring_val = readMuxMedian(LINEsensor_pin[i]);
    if (line_ring_val > max_line_value[i])
    {
      max_line_value[i] = line_ring_val;
    }

    if (line_ring_val > LINEsensor_adjust_value[i]) // ラインが見えたら
      lines_data_bit_mask |= (1UL << i);
  }

  // サイドライン
  uint32_t line_side_val[3] = {readAnalogMedian(LINEsensor_side_right_pin), readAnalogMedian(LINEsensor_side_left_pin), readAnalogMedian(LINEsensor_side_back_pin)};
  if (line_side_val[0] >= LINEsensor_side_right_adjust_value)
  {
    lines_data_bit_mask |= (1UL << 16);
  }
  if (line_side_val[1] >= LINEsensor_side_left_adjust_value)
  {
    lines_data_bit_mask |= (1UL << 17);
  }
  if (line_side_val[2] >= LINEsensor_side_back_adjust_value)
  {
    lines_data_bit_mask |= (1UL << 18);
  }

  if (line_side_val[0] > max_line_value[16])
  {
    max_line_value[16] = line_side_val[0];
  }
  if (line_side_val[1] > max_line_value[17])
  {
    max_line_value[17] = line_side_val[1];
  }
  if (line_side_val[2] > max_line_value[18])
  {
    max_line_value[18] = line_side_val[2];
  }

  // 送信
  Serial1.write(start_header);                                  // teensyとの通信開始
  Serial1.write((uint8_t)(lines_data_bit_mask & 0xFF));         // 3byteのデータなので下位の1byteのみ送信
  Serial1.write((uint8_t)((lines_data_bit_mask >> 8) & 0xFF));  // 3byteのデータなので中位の1byteのみ送信
  Serial1.write((uint8_t)((lines_data_bit_mask >> 16) & 0xFF)); // 3byteのデータなので上位の1byteを送信
  Serial1.write(end_header);                                    // teensyとの通信終了
  Serial1.flush();

  // 閾値調整
// #define ADJUST
#ifdef ADJUST
  const uint32_t ring_add = 50;
  const uint32_t side_add = 50;

  // エンジェルライン
  Serial.print("{");
  for (int i = 0; i < 16; i++)
  {
    uint32_t a = max_line_value[i] + ring_add;
    Serial.print(String(a) + ", ");
  }
  Serial.print("} ");

  // サイドライン
  Serial.print("{");
  for (int i = 16; i < 19; i++)
  {
    uint32_t a = max_line_value[i] + side_add;
    Serial.print(String(a) + ", ");
  }
  Serial.println("}");
#endif

  delay(1);
}