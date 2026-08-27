#include <Arduino.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include "module/lidar.hpp"
#include "common/serial_packet.hpp"

struct t_data
{
  int16_t x = 45, y = 45; // コートの中心に対する自分の座標(x,y)
} __attribute__((packed));
struct r_data
{
  int16_t gyro_deg = 0; // ロボットの傾き
} __attribute__((packed));
serial_packet<t_data, r_data> packet;

class calc_posi
{
private:
  uint16_t _width = 0;     // コートの横幅
  uint16_t _height = 0;    // コートの縦幅
  uint16_t _allow_dis = 0; // 許容誤差

  float sin_table[360] = {0}, cos_table[360] = {0}; // sin,cosのテーブル

  int16_t _x[360] = {0}, _y[360] = {0};           // 絶対座標系での点群(x,y)
  int16_t _sort_x[360] = {0}, _sort_y[360] = {0}; // 昇順にソートした点群(x,y)
  int16_t _yfw[360] = {0}, _ybw[360] = {0};       // コートの前壁,後壁のy座標
  int16_t _xlw[360] = {0}, _xrw[360] = {0};       // コートの左壁,右壁のx座標
  int16_t _exw_idx[360] = {0};                    // _yfw,_ybw,_xlf,_xrfのうちどれにも属さないもののインデックス番号
  int16_t _exw_invalid_idx[360] = {0};            // _exw_idxを探索する際に無効とするインデックス番号
  int16_t _xc = 0, _yc = 0;                       // 自分に対するコートの中心座標(x,y)

  int16_t _idx(int16_t world_deg, int16_t gyro_deg) // 絶対座標系でのインデックス番号を求める
  {
    return (((world_deg - gyro_deg) % 360) + 360) % 360;
  }

  uint16_t _diff(int16_t a, int16_t b) // 差の絶対値を求める
  {
    return abs(a - b);
  }

  int16_t _allowsum(int16_t *sort_a, uint16_t length, uint8_t n) // 長さ(length)との差が許容誤差(_allow_dis)であるものの合計を求める
  {
    for (int i = 0; i < n; i++)
    {
      for (int j = 0; j < n; j++)
      {
        if (_diff(sort_a[359 - j] - sort_a[i], length) <= _allow_dis)
        {
          return sort_a[i] + sort_a[359 - j];
        }
      }
    }
    return FAILED;
  }

  uint16_t _minidx(uint16_t *lidar_dis, int16_t *idx_arr, int16_t idx_arr_size, int16_t *invalid_idx_arr, int16_t invalid_idx_arr_size)
  {
    uint16_t min_dis = UNDETECTED;
    uint16_t min_idx = UNDETECTED;
    for (int i = 0; i < idx_arr_size; i++)
    {
      int16_t idx = idx_arr[i];
      // invalid_idx_arrにidxが含まれている場合探索を中断する
      if (std::find(invalid_idx_arr, invalid_idx_arr + invalid_idx_arr_size, idx) != invalid_idx_arr + invalid_idx_arr_size)
        continue;
      if (min_dis == UNDETECTED || lidar_dis[idx] < min_dis)
      {
        min_dis = lidar_dis[idx];
        min_idx = idx;
      }
    }
    return min_idx;
  }

public:
  int16_t x = 0, y = 0;                           // コートの中心に対する自分の座標(x,y)
  int16_t enemy_count = 0;                        // 敵の数
  int16_t enemy_x[360] = {0}, enemy_y[360] = {0}; // コートの中心に対する敵の座標(x,y)

  calc_posi(uint16_t width, uint16_t height, uint16_t allow_dis) // コンストラクタ
  {
    _width = width;
    _height = height;
    _allow_dis = allow_dis;

    // sin,cosのテーブルを計算
    for (int i = 0; i < 360; i++)
    {
      sin_table[i] = sin(radians(i));
      cos_table[i] = cos(radians(i));
    }
  }

  void calc(uint16_t *lidar_dis, int16_t gyro_deg) // 計算
  {
    // 点群の座標を計算
    for (int i = 0; i < 360; i++)
    {
      uint16_t l_dis = lidar_dis[_idx(i, gyro_deg)];

      float c = cos_table[i];
      float s = sin_table[i];

      // 右,前をそれぞれx軸正の方向,y軸正の方向とする
      _x[i] = (int16_t)(-s * l_dis);
      _sort_x[i] = _x[i];
      _y[i] = (int16_t)(c * l_dis);
      _sort_y[i] = _y[i];
    }

    // 点群を昇順にソート
    std::sort(_sort_x, _sort_x + 360);
    std::sort(_sort_y, _sort_y + 360);

    // 仮コート中心 仮壁座標を計算
    int16_t xsum = _allowsum(_sort_x, _width, 10);
    int16_t ysum = _allowsum(_sort_y, _height, 10);

    if (xsum == FAILED && ysum == FAILED) // 壁が検出されなかった場合は計算を中止
      return;
    bool xc_prime_failed = (xsum == FAILED), yc_prime_failed = (ysum == FAILED);

    int16_t xc_prime = xsum / 2; // 仮コートの中心のx座標
    int16_t yc_prime = ysum / 2; // 仮コートの中心のy座標

    int16_t yfw_prime = yc_prime + _height / 2; // 仮コートの前壁のy座標
    int16_t ybw_prime = yc_prime - _height / 2; // 仮コートの後壁のy座標
    int16_t xlw_prime = xc_prime - _width / 2;  // 仮コートの左壁のx座標
    int16_t xrw_prime = xc_prime + _width / 2;  // 仮コートの右壁のx座標

    // 壁を表す点群w系に格納
    uint8_t yfw_count = 0, ybw_count = 0, xlw_count = 0, xrw_count = 0, exw_count = 0;
    for (int i = 0; i < 360; i++)
    {
      int32_t d[4] = {yfw_prime - _y[i],
                      _y[i] - ybw_prime,
                      _x[i] - xlw_prime,
                      xrw_prime - _x[i]};
      if (d[0] < 0 || d[1] < 0 || d[2] < 0 || d[3] < 0) // lidarの異常値であるため除外
        continue;
      long abs_d[4] = {abs(d[0]),
                       abs(d[1]),
                       abs(d[2]),
                       abs(d[3])};
      uint32_t min_index = std::min_element(abs_d, abs_d + 4) - abs_d;

      if (d[min_index] > _allow_dis && d[min_index] > ROBO_RADIOUS) // どの壁にも属さない点群はexw系に格納
      {
        _exw_idx[exw_count] = i;
        _exw_invalid_idx[exw_count] = i;
        exw_count++;
        continue;
      }

      switch (min_index)
      {
      case 0:
        _yfw[yfw_count] = _y[i];
        yfw_count++;
        break;
      case 1:
        _ybw[ybw_count] = _y[i];
        ybw_count++;
        break;
      case 2:
        _xlw[xlw_count] = _x[i];
        xlw_count++;
        break;
      default: // case 3
        _xrw[xrw_count] = _x[i];
        xrw_count++;
        break;
      }
    }

    if (!(yfw_count || ybw_count) && !(xlw_count || xrw_count)) // 壁が検出されなかった場合は計算を中止
      return;

    // コートの中心座標を計算
    int32_t xw_sum = 0, yw_sum = 0;
    uint8_t xw_max = max(xlw_count, xrw_count), yw_max = max(yfw_count, ybw_count);
    for (int i = 0; i < xw_max; i++)
    {
      if (i < xlw_count)
        xw_sum += _xlw[i];
      if (i < xrw_count)
        xw_sum += _xrw[i];
    }
    for (int i = 0; i < yw_max; i++)
    {
      if (i < yfw_count)
        yw_sum += _yfw[i];
      if (i < ybw_count)
        yw_sum += _ybw[i];
    }

    int32_t xc_num = xw_sum + (int32_t)_width * ((int32_t)xlw_count - (int32_t)xrw_count) / 2;
    int32_t xc_den = xrw_count + xlw_count;
    if (xc_den != 0 && !xc_prime_failed)
      _xc = int16_t((xc_num + (xc_num >= 0 ? xc_den / 2 : -xc_den / 2)) / xc_den); // xc_num / xc_denの四捨五入

    int32_t yc_num = yw_sum + (int32_t)_height * ((int32_t)ybw_count - (int32_t)yfw_count) / 2;
    int32_t yc_den = yfw_count + ybw_count;
    if (yc_den != 0 && !yc_prime_failed)
      _yc = int16_t((yc_num + (yc_num >= 0 ? yc_den / 2 : -yc_den / 2)) / yc_den); // yc_num / yc_denの四捨五入

    // コートの中心に対する自分の座標を計算
    x = -_xc;
    y = -_yc;
  }
};

calc_posi posi(1800, 2430, 150); // コートの横幅,縦幅,許容誤差

void setup()
{
  // Serial.begin(9600); // pc

  Serial1.setRX(29);
  Serial1.setTX(28);
  Serial1.begin(115200);
  packet.begin(Serial1); // STM32との通信

  Serial2.setRX(5);
  Serial2.setTX(4);
  Serial2.begin(230400);
  lidar::attach(Serial2); // lidarとの通信
}

void loop()
{
  // STM32のデータを受送信
  packet.update();

  // lidarデータを更新
  lidar::process();

  static uint8_t local_cycle = 0;
  if (local_cycle != lidar::cycle)
  {
    local_cycle = lidar::cycle;
    posi.calc(lidar::dis, packet.rx.gyro_deg); // 自分の座標を計算

    packet.tx.x = 45;
    packet.tx.y = 45;

    // Serial.print(posi.x);
    // Serial.print(",");
    // Serial.println(posi.y);
  }
}

void setup1()
{
}

void loop1()
{
}