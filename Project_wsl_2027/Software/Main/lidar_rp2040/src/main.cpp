#include <Arduino.h>
#include <math.h>
#include "module/lidar.hpp"

class check_posi
{
private:
  uint16_t _x_limit = 0;   // コートの横幅
  uint16_t _y_limit = 0;   // コートの縦幅
  uint16_t _allow_dis = 0; // 許容誤差
  uint16_t _front[91];     // ポジション
  uint16_t _left[91];      // ポジション
  uint16_t _rear[91];      // ポジション
  uint16_t _right[91];     // ポジション

  int16_t idx(int16_t abs_deg, int16_t gyro_deg) // コート上の絶対角度のインデックスを取得
  {
    return (abs_deg - gyro_deg + 360) % 360;
  }

  uint16_t diff(uint16_t a, uint16_t b)
  {
    return abs(a - b);
  }

  int16_t posi(uint16_t d1, uint16_t d2, uint16_t limit)
  {
    uint16_t scaled_d1 = d1; //* limit / (d1 + d2);

    return int16_t(limit / 2 - scaled_d1);
  }

  bool ok(uint16_t diff)
  {
    return (diff <= _allow_dis);
  }

public:
  int16_t x = 0, y = 0;

  check_posi(uint16_t x_limit, uint16_t y_limit, uint16_t allow_dis)
  {
    _x_limit = x_limit;
    _y_limit = y_limit;
    _allow_dis = allow_dis;
  }

  void calc(uint16_t *lidar_dis, int16_t gyro_deg)
  {
    bool x_ok = false, y_ok = false;
    int x1_idx = 45, x2_idx = 45;
    int y1_idx = 45, y2_idx = 45;

    _front[45] = lidar_dis[idx(0, gyro_deg)];
    _left[45] = lidar_dis[idx(90, gyro_deg)];
    _rear[45] = lidar_dis[idx(180, gyro_deg)];
    _right[45] = lidar_dis[idx(270, gyro_deg)];

    if (ok(diff(_front[45] + _rear[45], _y_limit)))
    {
      y1_idx = y2_idx = 45;
      y_ok = true;
    }
    if (ok(diff(_left[45] + _right[45], _x_limit)))
    {
      x1_idx = x2_idx = 45;
      x_ok = true;
    }

    if (x_ok && y_ok)
    {
      x = posi(_left[45], _right[45], _x_limit);
      y = posi(_front[45], _rear[45], _y_limit);
      return;
    }

    for (int i = 0; i <= 90; i++)
    {
      if (i == 45)
        continue;

      if (!y_ok)
      {
        int16_t front_abs_deg = (i - 45 + 360) % 360;
        int16_t rear_abs_deg = (i + 135) % 360;

        _front[i] = abs(lidar_dis[idx(front_abs_deg, gyro_deg)] * cos(radians(front_abs_deg)));
        _rear[i] = abs(lidar_dis[idx(rear_abs_deg, gyro_deg)] * cos(radians(rear_abs_deg)));
      }

      if (!x_ok)
      {
        int16_t left_abs_deg = (i + 45) % 360;
        int16_t right_abs_deg = (i + 225) % 360;

        _left[i] = abs(lidar_dis[idx(left_abs_deg, gyro_deg)] * sin(radians(left_abs_deg)));
        _right[i] = abs(lidar_dis[idx(right_abs_deg, gyro_deg)] * sin(radians(right_abs_deg)));
      }
    }

    uint16_t diff_log_x = diff(_left[45] + _right[45], _x_limit), diff_log_y = diff(_front[45] + _rear[45], _y_limit);
    for (int a = 0; a <= 90 && (!x_ok || !y_ok); a++)
    {
      for (int b = 0; b <= 90; b++)
      {
        if (a == 45 && b == 45)
          continue;

        uint16_t diff_x = diff(_left[a] + _right[b], _x_limit);
        uint16_t diff_y = diff(_front[a] + _rear[b], _y_limit);

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

    x = posi(_left[x1_idx], _right[x2_idx], _x_limit);
    y = posi(_front[y1_idx], _rear[y2_idx], _y_limit);

    return;
  }
};

check_posi posi(1800, 2430, 150);

void setup()
{
  Serial.begin(115200);
  Serial1.begin(230400);
  lidar::attach(Serial1);
}

void loop()
{
  lidar::process();

  static uint32_t last_time = 0;
  if (millis() - last_time > 5)
  {
    posi.calc(lidar::dis, 0);
    Serial.print(posi.x);
    Serial.print(" ");
    Serial.println(posi.y);

    last_time = millis();
  }
}