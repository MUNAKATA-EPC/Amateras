#include <Arduino.h>
// #include <math.h>
#include <iostream>
#include <algorithm>
#include "module/lidar.hpp"
#include "common/serial_packet.hpp"

// gyro_degが右回転で正なら-1、左回転で正なら+1
static constexpr int16_t gyro_sign = -1;

struct t_data
{
  int16_t x = 45, y = 45;           // コートの中心に対する自分の座標(x,y)
  int16_t enemy_x = 0, enemy_y = 0; // コート中心に対する敵の座標(enemy_x,enemy_y)
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
  bool _valid[360] = {false};
  int16_t _x_history[3] = {0}, _y_history[3] = {0};
  uint8_t _x_history_count = 0, _y_history_count = 0;
  uint8_t _x_history_next = 0, _y_history_next = 0;
  bool _x_history_initialized = false, _y_history_initialized = false;

  int16_t _idx(int16_t world_deg, int16_t gyro_deg) // 絶対座標系でのインデックス番号を求める
  {
    int32_t idx = (int32_t)world_deg - (int32_t)gyro_sign * (int32_t)gyro_deg;
    return (int16_t)(((idx % 360) + 360) % 360);
  }

  uint16_t _diff(int16_t a, int16_t b) // 差の絶対値を求める
  {
    return abs(a - b);
  }

  int16_t _allowsum(int16_t *sort_a, uint16_t count, uint16_t length, uint8_t n) // 長さ(length)との差が許容誤差(_allow_dis)であるものの合計を求める
  {
    if (count < 20)
      return FAILED;

    int16_t centers[100];
    uint8_t center_count = 0;
    uint16_t tolerance = std::max<uint16_t>(_allow_dis, length / 10);
    uint8_t edge_count = std::min<uint16_t>(n, count / 2);

    for (int i = 0; i < edge_count; i++)
    {
      for (int j = 0; j < edge_count; j++)
      {
        int32_t measured = (int32_t)sort_a[count - 1 - j] - sort_a[i];
        if (measured >= 0 && _diff((int16_t)measured, length) <= tolerance)
        {
          centers[center_count++] = (int16_t)(((int32_t)sort_a[i] + sort_a[count - 1 - j]) / 2);
        }
      }
    }

    if (center_count < 3)
      return FAILED;

    std::sort(centers, centers + center_count);
    uint16_t center_tolerance = std::max<uint16_t>(50, _allow_dis / 2);
    uint8_t best_start = 0, best_count = 0;
    for (uint8_t i = 0; i < center_count; i++)
    {
      uint8_t j = i;
      while (j < center_count && (int32_t)centers[j] - centers[i] <= center_tolerance)
        j++;
      if (j - i > best_count)
      {
        best_start = i;
        best_count = j - i;
      }
    }
    if (best_count < 3)
      return FAILED;
    return centers[best_start + best_count / 2] * 2;
  }

  int16_t _trimmed_mean(int16_t *values, uint16_t count)
  {
    if (count == 0)
      return 0;
    std::sort(values, values + count);
    uint16_t trim = count / 5;
    if (trim * 2 >= count)
      trim = 0;
    int32_t sum = 0;
    for (uint16_t i = trim; i < count - trim; i++)
      sum += values[i];
    uint16_t used = count - trim * 2;
    return (int16_t)((sum + (sum >= 0 ? used / 2 : -(int32_t)used / 2)) / used);
  }

  int16_t _filter_axis(int16_t raw, int16_t *history, uint8_t &count, uint8_t &next, bool &initialized, int16_t current)
  {
    history[next] = raw;
    next = (next + 1) % 3;
    if (count < 3)
      count++;

    int16_t sorted[3];
    for (uint8_t i = 0; i < count; i++)
      sorted[i] = history[i];
    std::sort(sorted, sorted + count);
    int16_t median = sorted[count / 2];
    if (!initialized)
    {
      initialized = true;
      return median;
    }
    return (int16_t)(current + ((int32_t)(median - current) * 50) / 100);
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
    uint16_t point_count = 0;
    for (int i = 0; i < 360; i++)
    {
      uint16_t l_dis = lidar_dis[_idx(i, gyro_deg)];
      _valid[i] = false;
      if (l_dis == 0 || l_dis == UNDETECTED || l_dis <= ROBO_RADIOUS)
        continue;

      float c = cos_table[i];
      float s = sin_table[i];

      // 右,前をそれぞれx軸正の方向,y軸正の方向とする
      _x[i] = (int16_t)(-s * l_dis);
      _y[i] = (int16_t)(c * l_dis);
      _valid[i] = true;
      _sort_x[point_count] = _x[i];
      _sort_y[point_count] = _y[i];
      point_count++;
    }

    if (point_count < 30)
      return;

    // 点群を昇順にソート
    std::sort(_sort_x, _sort_x + point_count);
    std::sort(_sort_y, _sort_y + point_count);

    // 仮コート中心 仮壁座標を計算
    int16_t xsum = _allowsum(_sort_x, point_count, _width, 10);
    int16_t ysum = _allowsum(_sort_y, point_count, _height, 10);

    if (xsum == FAILED && ysum == FAILED && !_x_history_initialized && !_y_history_initialized) // 壁が検出されなかった場合は計算を中止
      return;

    int16_t xc_prime = xsum == FAILED ? _xc : xsum / 2; // 仮コートの中心のx座標
    int16_t yc_prime = ysum == FAILED ? _yc : ysum / 2; // 仮コートの中心のy座標

    int16_t yfw_prime = yc_prime + _height / 2; // 仮コートの前壁のy座標
    int16_t ybw_prime = yc_prime - _height / 2; // 仮コートの後壁のy座標
    int16_t xlw_prime = xc_prime - _width / 2;  // 仮コートの左壁のx座標
    int16_t xrw_prime = xc_prime + _width / 2;  // 仮コートの右壁のx座標

    // 壁を表す点群w系に格納
    uint16_t yfw_count = 0, ybw_count = 0, xlw_count = 0, xrw_count = 0, exw_count = 0;
    int32_t x_tolerance = std::max<uint16_t>(_allow_dis, _width / 10);
    int32_t y_tolerance = std::max<uint16_t>(_allow_dis, _height / 10);
    for (int i = 0; i < 360; i++)
    {
      if (!_valid[i])
        continue;
      int32_t d[4] = {yfw_prime - _y[i],
                      _y[i] - ybw_prime,
                      _x[i] - xlw_prime,
                      xrw_prime - _x[i]};
      int32_t tolerances[4] = {y_tolerance, y_tolerance, x_tolerance, x_tolerance};
      if (d[0] < -tolerances[0] || d[1] < -tolerances[1] || d[2] < -tolerances[2] || d[3] < -tolerances[3]) // lidarの異常値であるため除外
        continue;
      long abs_d[4] = {abs(d[0]),
                       abs(d[1]),
                       abs(d[2]),
                       abs(d[3])};
      uint32_t min_index = std::min_element(abs_d, abs_d + 4) - abs_d;

      if (abs_d[min_index] > tolerances[min_index] && abs_d[min_index] > ROBO_RADIOUS) // どの壁にも属さない点群はexw系に格納
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

    if (!(yfw_count || ybw_count) && !(xlw_count || xrw_count) && xsum == FAILED && ysum == FAILED) // 壁が検出されなかった場合は計算を中止
      return;

    // コートの中心座標を計算
    int32_t xw_sum = 0, yw_sum = 0;
    if (xlw_count)
      xw_sum += (int32_t)_trimmed_mean(_xlw, xlw_count) * xlw_count;
    if (xrw_count)
      xw_sum += (int32_t)_trimmed_mean(_xrw, xrw_count) * xrw_count;
    if (yfw_count)
      yw_sum += (int32_t)_trimmed_mean(_yfw, yfw_count) * yfw_count;
    if (ybw_count)
      yw_sum += (int32_t)_trimmed_mean(_ybw, ybw_count) * ybw_count;

    int32_t xc_num = xw_sum + (int32_t)_width * ((int32_t)xlw_count - (int32_t)xrw_count) / 2;
    int32_t xc_den = xrw_count + xlw_count;
    bool x_estimated = xsum != FAILED;
    if (xsum != FAILED)
      _xc = xsum / 2;
    if (xc_den >= 3)
    {
      _xc = int16_t((xc_num + (xc_num >= 0 ? xc_den / 2 : -xc_den / 2)) / xc_den); // xc_num / xc_denの四捨五入
      x_estimated = true;
    }

    int32_t yc_num = yw_sum + (int32_t)_height * ((int32_t)ybw_count - (int32_t)yfw_count) / 2;
    int32_t yc_den = yfw_count + ybw_count;
    bool y_estimated = ysum != FAILED;
    if (ysum != FAILED)
      _yc = ysum / 2;
    if (yc_den >= 3)
    {
      _yc = int16_t((yc_num + (yc_num >= 0 ? yc_den / 2 : -yc_den / 2)) / yc_den); // yc_num / yc_denの四捨五入
      y_estimated = true;
    }

    // コートの中心に対する自分の座標を計算
    if (x_estimated)
      x = _filter_axis(-_xc, _x_history, _x_history_count, _x_history_next, _x_history_initialized, x);
    if (y_estimated)
      y = _filter_axis(-_yc, _y_history, _y_history_count, _y_history_next, _y_history_initialized, y);
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

    packet.tx.x = posi.x;
    packet.tx.y = posi.y;
  }
}

void setup1()
{
}

void loop1()
{
}
