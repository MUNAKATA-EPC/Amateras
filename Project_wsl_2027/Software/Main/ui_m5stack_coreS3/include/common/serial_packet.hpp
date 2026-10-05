#pragma once
#include <Arduino.h>

template <class tx_t, class rx_t>
class serial_packet
{
private:
  HardwareSerial *_serial = nullptr;
  const uint8_t _start_byte = 0xAA;

  // RX
  int _rx_state = 0;
  size_t _rx_index = 0;
  uint8_t _expected_size = 0;
  uint8_t _calc_checksum = 0;
  uint8_t _rx_buffer[sizeof(rx_t) > 0 ? sizeof(rx_t) : 1] = {};

  // TX
  uint8_t _tx_buffer[2 + sizeof(tx_t) + 1] = {};
  size_t _tx_buf_len = 0;
  size_t _tx_buf_sent = 0;
  bool _tx_pending = false;
  uint32_t _last_tx_time = 0;

public:
  tx_t tx;
  rx_t rx;

  serial_packet() : tx(), rx() {}

  void begin(HardwareSerial &serial_obj)
  {
    _serial = &serial_obj;
  }

  void reset()
  {
    _rx_state = 0;
    _rx_index = 0;
    _expected_size = 0;
    _calc_checksum = 0;
    _tx_pending = false;
    _tx_buf_sent = 0;
    if (_serial)
      while (_serial->available())
        _serial->read();
  }

  bool read()
  {
    if (!_serial)
      return false;
    return readPacket();
  }

  void send()
  {
    if (!_serial)
      return;

    // 前回の送信が終わっていなければ残りの送信だけ試みてスキップ
    if (_tx_pending)
    {
      flushTx();
      return;
    }

    // 送信バッファ組み立て
    size_t data_size = sizeof(tx_t);
    uint8_t *raw_ptr = (uint8_t *)&tx;
    uint8_t checksum = 0;

    _tx_buffer[0] = _start_byte;
    _tx_buffer[1] = (uint8_t)data_size;
    for (size_t i = 0; i < data_size; i++)
    {
      _tx_buffer[2 + i] = raw_ptr[i];
      checksum ^= raw_ptr[i];
    }
    _tx_buffer[2 + data_size] = checksum;
    _tx_buf_len = 2 + data_size + 1;
    _tx_buf_sent = 0;
    _tx_pending = true;

    flushTx();
  }

  bool update(uint32_t send_interval_ms = 20)
  {
    // 送信残りを毎回消化（ノンブロッキング）
    if (_tx_pending)
      flushTx();

    bool received = read();

    if (!_tx_pending && millis() - _last_tx_time >= send_interval_ms)
    {
      _last_tx_time = millis();
      send();
    }

    return received;
  }

private:
  void flushTx()
  {
    if (!_tx_pending || !_serial)
      return;

    while (_tx_buf_sent < _tx_buf_len)
    {
      int avail = _serial->availableForWrite();
      if (avail <= 0)
        break; // FIFOが満杯 → 今回は諦める

      size_t to_send = min((size_t)avail, _tx_buf_len - _tx_buf_sent);
      _serial->write(&_tx_buffer[_tx_buf_sent], to_send);
      _tx_buf_sent += to_send;
    }

    if (_tx_buf_sent >= _tx_buf_len)
    {
      _tx_pending = false;
    }
  }

  bool readPacket()
  {
    bool packet_received = false;
    while (_serial && _serial->available() > 0)
    {
      uint8_t b = _serial->read();
      switch (_rx_state)
      {
      case 0:
        if (b == _start_byte)
          _rx_state = 1;
        break;
      case 1:
        if (b == sizeof(rx_t))
        {
          _expected_size = b;
          _rx_index = 0;
          _calc_checksum = 0;
          _rx_state = 2;
        }
        else
        {
          _rx_state = 0;
        }
        break;
      case 2:
        _rx_buffer[_rx_index++] = b;
        _calc_checksum ^= b;
        if (_rx_index >= _expected_size)
          _rx_state = 3;
        break;
      case 3:
        if (b == _calc_checksum)
        {
          memcpy(&rx, _rx_buffer, sizeof(rx_t));
          packet_received = true;
        }
        _rx_state = 0;
        break;
      }
    }
    return packet_received;
  }
};