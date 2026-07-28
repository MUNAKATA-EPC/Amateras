#include <Arduino.h>
#include <math.h>
#include "module/lidar.hpp"

class check_posi
{
private:
  uint16_t _length = 0;    // コートの縦幅
  uint16_t _width = 0;     // コートの横幅
  uint16_t _allow_dis = 0; // 許容誤差
  uint16_t _front[91];     // ポジション
  uint16_t _left[91];      // ポジション
  uint16_t _rear[91];      // ポジション
  uint16_t _right[91];     // ポジション

  int16_t idx(int16_t abs_deg, int16_t gyro_deg) // コート上の絶対角度のインデックスを取得
  {
    return (abs_deg - gyro_deg + 360) % 360;
  }

  int16_t diff(int16_t a, int16_t b)
  {
    return abs(a - b);
  }

  int16_t posi(int16_t d1, int16_t d2, int16_t limit)
  {
    return d1;
  }

  bool ok(int16_t diff)
  {
    return (diff <= _allow_dis);
  }

public:
  int16_t x = 0, y = 0;

  check_posi(uint16_t length, uint16_t width, u_int16_t allow_dis)
  {
    _length = length;
    _width = width;
    _allow_dis = allow_dis;
  }

  void calc(uint16_t *lidar_dis, int16_t gyro_deg)
  {
    bool x_ok = false, y_ok = false;
    int x1_idx = 45, x2_idx = 45;
    int y1_idx = 45, y2_idx = 45;

    _front[45] = lidar_dis[0];
    _left[45] = lidar_dis[90];
    _rear[45] = lidar_dis[180];
    _right[45] = lidar_dis[270];

    if (ok(diff(_front[45], _rear[45])))
    {
      y1_idx = y2_idx = 45;
      y_ok = true;
    }
    if (ok(diff(_left[45], _right[45])))
    {
      x1_idx = x2_idx = 45;
      x_ok = true;
    }

    if (x_ok && y_ok)
    {
      x = posi(_front[45], _rear[45], _width);
      y = posi(_left[45], _right[45], _length);
      return;
    }

    for (int i = 0; i <= 90; i++)
    {
      if (i == 45)
        continue;

      if (!y_ok)
      {
        int front_abs_deg = (i - 45 + 360) % 360;
        int rear_abs_deg = (i + 135) % 360;

        _front[i] = lidar_dis[idx(front_abs_deg, gyro_deg)] * cos(radians(front_abs_deg));
        _rear[i] = -lidar_dis[idx(rear_abs_deg, gyro_deg)] * cos(radians(rear_abs_deg));
      }

      if (!x_ok)
      {
        int left_abs_deg = (i + 45) % 360;
        int right_abs_deg = (i + 225) % 360;

        _left[i] = lidar_dis[idx(left_abs_deg, gyro_deg)] * sin(radians(left_abs_deg));
        _right[i] = -lidar_dis[idx(right_abs_deg, gyro_deg)] * sin(radians(right_abs_deg));
      }
    }

    int16_t diff_log_x = diff(_left[45], _right[45]), diff_log_y = diff(_front[45], _rear[45]);
    for (int a = 0; a < 90 && (!x_ok || !y_ok); a++)
    {
      for (int b = 0; b < 90; b++)
      {
        if (a == 45 && b == 45)
          continue;

        int diff_x = diff(_right[a], _left[b]);
        int diff_y = diff(_front[a], _rear[b]);

        if (diff_x < diff_log_x)
        {
          diff_log_x = diff_x;
          x1_idx = a;
          x2_idx = b;

          if (ok(diff_log_x))
          {
            x_ok = true;
          }
        }

        if (diff_y < diff_log_y)
        {
          diff_log_y = diff_y;
          y1_idx = a;
          y2_idx = b;

          if (ok(diff_log_y))
          {
            y_ok = true;
          }
        }
      }
    }

    x = posi(_front[x1_idx], _rear[x2_idx], _width);
    y = posi(_left[y1_idx], _right[y2_idx], _length);
    return;
  }
};

check_posi posi(2430, 1800, 50);

void setup()
{
  Serial.begin(115200);
  Serial1.begin(230400);
  lidarInit(Serial1);
}

void loop()
{
  lidarUpdate();

  static uint32_t last_time = 0;
  if (millis() - last_time > 5)
  {
    posi.calc(lidar_dis, 0);
    Serial.print(posi.x);
    Serial.print(" ");
    Serial.println(posi.y);

    last_time = millis();
  }
}