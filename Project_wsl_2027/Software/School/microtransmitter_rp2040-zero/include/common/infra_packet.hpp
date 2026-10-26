#pragma once

#ifndef INFRA_PACKET_CARRIER_KHZ
#define INFRA_PACKET_CARRIER_KHZ 38
#endif

#ifndef INFRA_PACKET_UNIT_US
#define INFRA_PACKET_UNIT_US 500
#endif

// フレーム全体 (16 * (payload + 3) + 3 エントリ) が受信バッファに収まるようにする
#if defined(INFRA_PACKET_MAX_PAYLOAD) && !defined(RAW_BUFFER_LENGTH)
#define RAW_BUFFER_LENGTH (16 * (INFRA_PACKET_MAX_PAYLOAD + 3) + 6)
#endif

#include <Arduino.h>
#include <IRremote.hpp>

template <class tx_t, class rx_t>
class infra_packet
{
private:
  const uint8_t _start_byte = 0xAA;

  // 信号の形 [us]
  enum : uint32_t
  {
    _unit = INFRA_PACKET_UNIT_US,
    _header_mark = 8 * _unit,
    _header_space = 4 * _unit,
    _bit_mark = _unit,
    _zero_space = _unit,
    _one_space = 3 * _unit,

    // 連続して送ってもフレームがつながって見えない最小の無音時間 [ms]
    _min_gap_ms = (RECORD_GAP_MICROS + 999) / 1000 + 4,

    // 受信側の判定しきい値 [IRremote の tick = MICROS_PER_TICK us]
    _t_header_mark_min = _header_mark * 3 / 4 / MICROS_PER_TICK,
    _t_header_mark_max = _header_mark * 5 / 4 / MICROS_PER_TICK + 1,
    _t_header_space_min = _header_space * 3 / 4 / MICROS_PER_TICK,
    _t_header_space_max = _header_space * 5 / 4 / MICROS_PER_TICK + 1,
    _t_bit_mark_min = _bit_mark * 2 / 5 / MICROS_PER_TICK,
    _t_bit_mark_max = _bit_mark * 2 / MICROS_PER_TICK + 1,
    _t_zero_space_min = _zero_space * 2 / 5 / MICROS_PER_TICK,
    _t_one_space_max = _one_space * 5 / 3 / MICROS_PER_TICK + 1,
    _t_one_threshold = _unit * 2 / MICROS_PER_TICK, // これ以上の space なら bit 1
  };

  static_assert(sizeof(tx_t) <= 255 && sizeof(rx_t) <= 255,
                "tx_t / rx_t must be 255 bytes or less");
  // rawlen = 2 * bits + 4 が RAW_BUFFER_LENGTH - 1 以下でないと IRremote が overflow 扱いにする
  static_assert(2 * 8 * (sizeof(rx_t) + 3) + 4 <= RAW_BUFFER_LENGTH - 1,
                "RAW_BUFFER_LENGTH is too small for rx_t: "
                "#define INFRA_PACKET_MAX_PAYLOAD (>= sizeof(rx_t)) before including infra_packet.h");

  bool _tx_enabled = false;
  bool _rx_enabled = false;

  uint8_t _rx_buffer[sizeof(rx_t) > 0 ? sizeof(rx_t) : 1] = {};

  uint32_t _tx_interval_ms = 0; // update() が送信する最小間隔
  uint32_t _last_tx_time = 0;   // 直近の送信が終わった時刻 [ms]
  uint32_t _next_tx_time = 0;   // update() が次に送信してよい時刻 [ms]

public:
  tx_t tx;
  rx_t rx;

  infra_packet() : tx(), rx() {}

  // 受信ピンがあるとき (双方向) は、送信の間に相手が1フレーム送れる分の待ち時間を自動で空ける
  void begin(int send_pin, int recv_pin, uint32_t tx_interval_ms = 0)
  {
    _tx_enabled = (send_pin >= 0);
    _rx_enabled = (recv_pin >= 0);

    if (_tx_enabled)
    {
#if defined(IR_SEND_PIN)
      (void)send_pin; // 送信ピンはコンパイル時に固定されている
      IrSender.begin();
#else
      IrSender.begin((uint8_t)send_pin);
#endif
    }
    if (_rx_enabled)
      IrReceiver.begin((uint8_t)recv_pin);

    if (tx_interval_ms > 0)
    {
      _tx_interval_ms = tx_interval_ms;
      _last_tx_time = millis();
      _next_tx_time = _last_tx_time + tx_interval_ms;
    }
    else
    {
      _tx_interval_ms = _rx_enabled ? frame_ms(sizeof(rx_t)) + 20 : 0;
      _last_tx_time = millis();
      _next_tx_time = _last_tx_time + jitter();
    }
  }

  // 受信済みで未読のフレームを破棄する
  void reset()
  {
    if (_rx_enabled)
      IrReceiver.resume();
  }

  bool read()
  {
    if (!_tx_enabled && !_rx_enabled)
      return false;
    return read_packet();
  }

  // 1フレーム送信する
  void send()
  {
    if (!_tx_enabled)
      return;

    constexpr size_t data_size = sizeof(tx_t);
    uint8_t send_buf[2 + data_size + 1];
    uint8_t *raw_ptr = (uint8_t *)&tx;
    uint8_t checksum = 0;

    send_buf[0] = _start_byte;
    send_buf[1] = (uint8_t)data_size;

    for (size_t i = 0; i < data_size; i++)
    {
      send_buf[2 + i] = raw_ptr[i];
      checksum ^= raw_ptr[i];
    }
    send_buf[2 + data_size] = checksum;

    // 前のフレームと連結して見えないように最低限の無音時間を確保する
    uint32_t since = millis() - _last_tx_time;
    if (since < _min_gap_ms)
      delay(_min_gap_ms - since);

    if (_rx_enabled)
      IrReceiver.stopTimer(); // 送信中は受信を止める (自分の光を拾わない / AVRではタイマー共有)

    IrSender.enableIROut(INFRA_PACKET_CARRIER_KHZ);
    IrSender.mark(_header_mark);
    IrSender.space(_header_space);
    for (size_t i = 0; i < sizeof(send_buf); i++)
    {
      for (uint8_t bit = 0; bit < 8; bit++)
      {
        IrSender.mark(_bit_mark);
        IrSender.space(((send_buf[i] >> bit) & 1) ? _one_space : _zero_space);
      }
    }
    IrSender.mark(_bit_mark); // 終端 (最後の space の長さを受信側に測らせる)
    IrSender.IRLedOff();

    if (_rx_enabled)
      IrReceiver.restartTimer(); // 未読の受信フレームは保ったまま再開

    _last_tx_time = millis();
    _next_tx_time = _last_tx_time + _tx_interval_ms + jitter();
  }

  // read() してから、送信してよい状況なら send() する
  bool update()
  {
    bool received = read();
    if (tx_ready())
      send();

    return received;
  }

private:
  // 1フレームの最悪(全ビット1)の長さ [us]
  static constexpr uint32_t frame_time_us(size_t payload)
  {
    return _header_mark + _header_space + 8UL * (payload + 3) * (_bit_mark + _one_space) + _bit_mark;
  }

  static constexpr uint32_t frame_ms(size_t payload)
  {
    return (frame_time_us(payload) + 999) / 1000;
  }

  // 双方が同じ周期で送り続けて衝突し続けないよう、送信間隔にばらつきを入れる (双方向のときだけ)
  uint32_t jitter() const
  {
    if (!_rx_enabled || _tx_interval_ms == 0)
      return 0;
    return (uint32_t)(micros() >> 2) % (_tx_interval_ms / 2 + 1);
  }

  bool tx_ready() const
  {
    if (!_tx_enabled)
      return false;

    uint32_t late = millis() - _next_tx_time; // 送信予定時刻からの経過
    if (late > 0x7FFFFFFFUL)
      return false; // まだ予定時刻になっていない
    if (!_rx_enabled || IrReceiver.isIdle())
      return true;

    // 相手が送信中 (受信中) は待つ。ただしノイズ等でずっと塞がっているなら諦めて送る
    return late >= 2 * frame_ms(sizeof(rx_t));
  }

  static bool in_range(uint16_t v, uint16_t lo, uint16_t hi)
  {
    return v >= lo && v <= hi;
  }

  // rawbuf の idx から8ビット(LSB first)読む。idx は次のバイトの先頭に進む
  static bool read_byte(const IRRawbufType *r, uint16_t &idx, uint8_t &out)
  {
    uint8_t v = 0;
    for (uint8_t bit = 0; bit < 8; bit++, idx += 2)
    {
      uint16_t m = r[idx];     // bit の mark
      uint16_t s = r[idx + 1]; // bit の space (長さで 0/1 が決まる)
      if (!in_range(m, _t_bit_mark_min, _t_bit_mark_max))
        return false;
      if (!in_range(s, _t_zero_space_min, _t_one_space_max))
        return false;
      if (s >= _t_one_threshold)
        v |= (uint8_t)(1 << bit);
    }
    out = v;
    return true;
  }

  // IRremote が受信し終えた1フレーム(rawbuf)を検証して rx に取り込む
  bool parse_frame()
  {
    const irparams_struct &p = IrReceiver.irparams;
    if (p.OverflowFlag || (uint16_t)p.rawlen != (uint16_t)(2 * 8 * (sizeof(rx_t) + 3) + 4))
      return false; // 他のリモコン / ノイズ / 長さ違い

    const IRRawbufType *r = p.rawbuf; // r[0] は未使用, r[1]=ヘッダmark, r[2]=ヘッダspace, 以降 mark/space の繰り返し
    if (!in_range(r[1], _t_header_mark_min, _t_header_mark_max) ||
        !in_range(r[2], _t_header_space_min, _t_header_space_max))
      return false;

    uint16_t idx = 3;
    uint8_t b = 0;
    uint8_t calc_checksum = 0;

    if (!read_byte(r, idx, b) || b != _start_byte)
      return false;
    if (!read_byte(r, idx, b) || b != (uint8_t)sizeof(rx_t))
      return false;
    for (size_t i = 0; i < sizeof(rx_t); i++)
    {
      if (!read_byte(r, idx, b))
        return false;
      _rx_buffer[i] = b;
      calc_checksum ^= b;
    }
    if (!read_byte(r, idx, b) || b != calc_checksum)
      return false;
    if (!in_range(r[idx], _t_bit_mark_min, _t_bit_mark_max)) // 終端の mark
      return false;

    memcpy(&rx, _rx_buffer, sizeof(rx_t));
    return true;
  }

  bool read_packet()
  {
    if (!_rx_enabled || !IrReceiver.available())
      return false; // まだ1フレーム受け終わっていない

    bool packet_received = parse_frame();
    IrReceiver.resume(); // rawbuf を読み終えたので次のフレームを受け付ける
    return packet_received;
  }
};