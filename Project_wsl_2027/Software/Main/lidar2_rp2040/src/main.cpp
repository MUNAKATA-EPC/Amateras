#include <Arduino.h>
#include <math.h>
#include <algorithm>
#include "module/lidar.hpp"
#include "common/set.hpp"

class posi
{
private:
  uint16_t _width = 0;     // コートの横幅
  uint16_t _height = 0;    // コートの縦幅
  uint16_t _allow_dis = 0; // 許容誤差

  int16_t _x[360] = {0}, _y[360] = {0};           // 絶対座標系での点群(x,y)
  int16_t _x_sort[360] = {0}, _y_sort[360] = {0}; // 絶対座標系での点群(x,y)
  int16_t xc = 0, yc = 0;                         // 自分に対するコートの中心座標(x,y)

  set<int16_t> Sf; // 前壁を表す点群(x,y)のインデックスの集合
  set<int16_t> Sb; // 後壁を表す点群(x,y)のインデックスの集合
  set<int16_t> Sl; // 左壁を表す点群(x,y)のインデックスの集合
  set<int16_t> Sr; // 右壁を表す点群(x,y)のインデックスの集合

  int16_t idx(int16_t abs_deg, int16_t gyro_deg) // 絶対座標系でのインデックス番号を求める
  {
    return (abs_deg - gyro_deg + 360) % 360;
  }

  uint16_t diff(int32_t a, int32_t b) // 差の絶対値を求める
  {
    return abs(a - b);
  }

public:
  int16_t x = 0, y = 0; // コートの中心に対する自分の座標(x,y)

  posi(uint16_t width, uint16_t height, uint16_t allow_dis) // コンストラクタ
  {
    _width = width;
    _height = height;
    _allow_dis = allow_dis;
  }

  void calc(uint16_t *lidar_dis, int16_t gyro_deg) // 計算
  {
    // 点群の座標を計算
    for (int i = 0; i < 360; i++)
    {
      uint16_t l_dis = lidar_dis[idx(i, gyro_deg)];

      float c = cos(radians(i));
      float s = sin(radians(i));

      // 右,前をそれぞれx軸正の方向,y軸正の方向とする
      _x[i] = (int16_t)(-s * l_dis);
      _y[i] = (int16_t)(c * l_dis);

      _x_sort[i] = _x[i];
      _y_sort[i] = _y[i];
    }
    // 昇順にソート
    std::sort(_x_sort, _x_sort + 360);
    std::sort(_y_sort, _y_sort + 360);

    // 仮でコートの中心座標を計算
    int16_t xc_prime = UNDETECTED, yc_prime = UNDETECTED;
    for (int a = 0; (a < 10) && (xc_prime == UNDETECTED || yc_prime == UNDETECTED); a++)
    {
      for (int b = 0; (b < 10) && (xc_prime == UNDETECTED || yc_prime == UNDETECTED); b++)
      {
        int32_t detected_width = (int32_t)_x_sort[359 - b] - (int32_t)_x_sort[a];
        int32_t detected_height = (int32_t)_y_sort[359 - b] - (int32_t)_y_sort[a];

        if (xc_prime == UNDETECTED && diff(detected_width, _width) < _allow_dis)
        {
          xc_prime = (int16_t)((_x_sort[a] + _x_sort[359 - b]) / 2);
        }

        if (yc_prime == UNDETECTED && diff(detected_height, _height) < _allow_dis)
        {
          yc_prime = (int16_t)((_y_sort[a] + _y_sort[359 - b]) / 2);
        }
      }
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Serial1.begin(230400);
  lidar::attach(Serial1);
}

void loop()
{
  // lidarデータを更新
  lidar::process();

  // 5ms周期でプリント
  static uint32_t last_time = millis();
  if (millis() - last_time > 5)
  {
    last_time = millis();
  }
}

void setup1()
{
}

void loop1()
{
}