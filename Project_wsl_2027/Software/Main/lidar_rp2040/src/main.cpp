#include <Arduino.h>
#include <math.h>
#include "module/lidar.hpp"

class check_my_posi
{
private:
  uint16_t _x_limit = 0;   // コートの横幅
  uint16_t _y_limit = 0;   // コートの縦幅
  uint16_t _allow_dis = 0; // 許容誤差
  uint16_t _front[91];     // ポジション
  uint16_t _left[91];      // ポジション
  uint16_t _rear[91];      // ポジション
  uint16_t _right[91];     // ポジション

  const uint8_t average_count = 10; // 10個の平均をとる

  int16_t idx(int16_t abs_deg, int16_t gyro_deg)
  {
    return (abs_deg - gyro_deg + 360) % 360;
  }

  uint16_t diff(uint16_t a, uint16_t b)
  {
    return abs(a - b);
  }

  int16_t posi(uint16_t d1, uint16_t d2, uint16_t limit)
  {
    uint16_t scaled_d1 = (uint32_t)d1 * limit / (d1 + d2);
    return int16_t(limit / 2 - scaled_d1);
  }

  int16_t average_posi(uint16_t *d1, uint16_t *d2, int *d1_idx, int *d2_idx, uint16_t limit, int &d_count)
  {
    uint32_t d1_sum = 0, d2_sum = 0;
    for (int i = 0; i < average_count; i++)
    {
      if (ok(diff(d1[d1_idx[i]] + d2[d2_idx[i]], limit)))
      {
        d1_sum += d1[d1_idx[i]];
        d2_sum += d2[d2_idx[i]];
        d_count++;
      }
    }

    uint16_t d1_ave = d_count == 0 ? d1[d1_idx[0]] : (d1_sum / d_count);
    uint16_t d2_ave = d_count == 0 ? d2[d2_idx[0]] : (d2_sum / d_count);

    return posi(d1_ave, d2_ave, limit);
  }

  bool ok(uint16_t diff)
  {
    return (diff <= _allow_dis);
  }

public:
  int16_t x = 0, y = 0;
  float x_confidence = 0.0f, y_confidence = 0.0f;

  check_my_posi(uint16_t x_limit, uint16_t y_limit, uint16_t allow_dis)
  {
    _x_limit = x_limit;
    _y_limit = y_limit;
    _allow_dis = allow_dis;
  }

  void calc(uint16_t *lidar_dis, int16_t gyro_deg)
  {
    int x1_idx[average_count], x2_idx[average_count];
    int y1_idx[average_count], y2_idx[average_count];
    uint16_t diff_log_x[average_count], diff_log_y[average_count];

    for (int i = 0; i < average_count; i++)
    {
      x1_idx[i] = 45;
      x2_idx[i] = 45;
      y1_idx[i] = 45;
      y2_idx[i] = 45;
      diff_log_x[i] = 0xFFFF;
      diff_log_y[i] = 0xFFFF;
    }

    _front[45] = lidar_dis[idx(0, gyro_deg)];
    _left[45] = lidar_dis[idx(90, gyro_deg)];
    _rear[45] = lidar_dis[idx(180, gyro_deg)];
    _right[45] = lidar_dis[idx(270, gyro_deg)];

    for (int i = 0; i <= 90; i++)
    {
      if (i == 45)
        continue;

      int16_t left_abs_deg = (i + 45) % 360;
      int16_t right_abs_deg = (i + 225) % 360;

      _left[i] = abs(lidar_dis[idx(left_abs_deg, gyro_deg)] * sin(radians(left_abs_deg)));
      _right[i] = abs(lidar_dis[idx(right_abs_deg, gyro_deg)] * sin(radians(right_abs_deg)));

      int16_t front_abs_deg = (i - 45 + 360) % 360;
      int16_t rear_abs_deg = (i + 135) % 360;

      _front[i] = abs(lidar_dis[idx(front_abs_deg, gyro_deg)] * cos(radians(front_abs_deg)));
      _rear[i] = abs(lidar_dis[idx(rear_abs_deg, gyro_deg)] * cos(radians(rear_abs_deg)));
    }

    for (int a = 0; a <= 90; a++)
    {
      for (int b = 0; b <= 90; b++)
      {
        uint16_t diff_x = diff(_left[a] + _right[b], _x_limit);

        for (int c = 0; c < average_count; c++)
        {
          if (diff_x < diff_log_x[c])
          {
            for (int d = average_count - 1; d > c; d--)
            {
              diff_log_x[d] = diff_log_x[d - 1];
              x1_idx[d] = x1_idx[d - 1];
              x2_idx[d] = x2_idx[d - 1];
            }
            diff_log_x[c] = diff_x;
            x1_idx[c] = a;
            x2_idx[c] = b;
            break;
          }
        }

        uint16_t diff_y = diff(_front[a] + _rear[b], _y_limit);

        for (int c = 0; c < average_count; c++)
        {
          if (diff_y < diff_log_y[c])
          {
            for (int d = average_count - 1; d > c; d--)
            {
              diff_log_y[d] = diff_log_y[d - 1];
              y1_idx[d] = y1_idx[d - 1];
              y2_idx[d] = y2_idx[d - 1];
            }
            diff_log_y[c] = diff_y;
            y1_idx[c] = a;
            y2_idx[c] = b;
            break;
          }
        }
      }
    }

    int x_count = 0;
    x = average_posi(_left, _right, x1_idx, x2_idx, _x_limit, x_count);
    x_confidence = 100.0f * x_count / average_count;

    int y_count = 0;
    y = average_posi(_front, _rear, y1_idx, y2_idx, _y_limit, y_count);
    y_confidence = 100.0f * y_count / average_count;
  }
};

class check_enemy_posi
{
private:
  uint16_t _x_limit = 0;   // コートの横幅
  uint16_t _y_limit = 0;   // コートの縦幅
  uint16_t _allow_dis = 0; // 許容誤差

  int16_t idx(int16_t abs_deg, int16_t gyro_deg)
  {
    return (abs_deg - gyro_deg + 360) % 360;
  }

  uint16_t diff(uint16_t a, uint16_t b)
  {
    return abs(a - b);
  }

  bool ok(uint16_t diff)
  {
    return (diff <= _allow_dis);
  }

  bool range(int16_t deg, int16_t from, int16_t to)
  {
    if (from > to)
      return deg >= from || deg <= to;
    else
      return deg >= from && deg <= to;
  }

  uint16_t realdis(int16_t *corner_deg, int16_t abs_deg, int16_t my_x, int16_t my_y)
  {
    float c = cos(radians(abs_deg));
    float s = sin(radians(abs_deg));

    if (range(abs_deg, corner_deg[1], corner_deg[0]))
      return (abs(c) < 0.01f) ? 0xFFFF : (uint16_t)abs((float)(_y_limit / 2 - my_y) / c);
    else if (range(abs_deg, corner_deg[2], corner_deg[1]))
      return (abs(s) < 0.01f) ? 0xFFFF : (uint16_t)abs((float)(-_x_limit / 2 - my_x) / s);
    else if (range(abs_deg, corner_deg[3], corner_deg[2]))
      return (abs(c) < 0.01f) ? 0xFFFF : (uint16_t)abs((float)(-_y_limit / 2 - my_y) / c);
    else
      return (abs(s) < 0.01f) ? 0xFFFF : (uint16_t)abs((float)(_x_limit / 2 - my_x) / s);
  }

public:
  int16_t x[3] = {0, 0, 0}, y[3] = {0, 0, 0};
  int enemy_count = 0;
  int16_t enemy_deg = 0; // 確認用（仮）
  int16_t diff_realdis[360];

  check_enemy_posi(uint16_t x_limit, uint16_t y_limit, uint16_t allow_dis)
  {
    _x_limit = x_limit;
    _y_limit = y_limit;
    _allow_dis = allow_dis;
  }

  void calc(int16_t my_x, int16_t my_y, uint16_t *lidar_dis, int16_t gyro_deg)
  {
    // y軸性の向きを0度と定める
    int16_t corner_deg[4] = {0, 0, 0, 0};
    corner_deg[0] = (90 - int16_t(degrees(atan2(_y_limit / 2 - my_y, _x_limit / 2 - my_x)) + 0.5f) + 360) % 360;   // 右前
    corner_deg[1] = (90 - int16_t(degrees(atan2(_y_limit / 2 - my_y, -_x_limit / 2 - my_x)) + 0.5f) + 360) % 360;  // 左前
    corner_deg[2] = (90 - int16_t(degrees(atan2(-_y_limit / 2 - my_y, -_x_limit / 2 - my_x)) + 0.5f) + 360) % 360; // 左後
    corner_deg[3] = (90 - int16_t(degrees(atan2(-_y_limit / 2 - my_y, _x_limit / 2 - my_x)) + 0.5f) + 360) % 360;  // 右後

    int16_t diff_realdis_max = _allow_dis;
    enemy_deg = UNDETECTED; // 敵が見つからなかった時のための無効値

    for (int i = 0; i < 360; i++)
    {
      if (i < 3)
      {
        x[i] = 0;
        y[i] = 0;
      }

      uint16_t l_dis = lidar_dis[idx(i, gyro_deg)];

      if (l_dis == 0)
      {
        diff_realdis[i] = 0;
        continue;
      }

      int16_t dr = (int16_t)realdis(corner_deg, i, my_x, my_y) - (int16_t)l_dis;
      diff_realdis[i] = dr;

      if (dr > 0 && dr > diff_realdis_max) // ← dr > 0 を追加
      {
        diff_realdis_max = dr;
        enemy_deg = i;
      }
    }
  }
};

check_my_posi my_posi(1800, 2430, 150);
check_enemy_posi enemy_posi(1800, 2430, 150);
volatile bool calc_finished = false; // volatile修飾子でloop1で最適化せず処理するようになる

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

  static uint8_t local_cycle = 0;
  if (local_cycle != lidar::cycle)
  {
    local_cycle = lidar::cycle;

    my_posi.calc(lidar::dis, 0); // 座標計算

    calc_finished = true;
  }

  // 5ms周期でプリント
  static uint32_t last_time = millis();
  if (millis() - last_time > 5)
  {
    Serial.print(my_posi.x);
    Serial.print("(");
    Serial.print(my_posi.x_confidence);
    Serial.print(") , ");
    Serial.print(my_posi.y);
    Serial.print("(");
    Serial.print(my_posi.y_confidence);
    Serial.print(") -> ");
    Serial.println(enemy_posi.enemy_deg);

    last_time = millis();
  }
}

void setup1()
{
}

void loop1()
{
  if (calc_finished)
  {
    calc_finished = false;

    // 敵座標計算
    enemy_posi.calc(my_posi.x, my_posi.y, lidar::dis, 0);
  }
}