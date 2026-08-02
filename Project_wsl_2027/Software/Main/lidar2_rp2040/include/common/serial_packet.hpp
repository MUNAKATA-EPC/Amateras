#pragma once
#include <Arduino.h>

template <class tx_t, class rx_t>
class serial_packet
{
private:
  HardwareSerial *_serial = nullptr;
  const uint8_t _start_byte = 0xAA;

  int _rx_state = 0;
  size_t _rx_index = 0;
  uint8_t _expected_size = 0;
  uint8_t _calc_checksum = 0;
  uint8_t _rx_buffer[sizeof(rx_t) > 0 ? sizeof(rx_t) : 1] = {};

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

    if (_serial)

      while (_serial->available())

        _serial->read();

  }

  bool read()
  {
    if (!_serial) return false;
    return readPacket();
  }

  void send()
  {
    if (!_serial) return;
    
    size_t data_size = sizeof(tx_t);
    uint8_t send_buf[2 + data_size + 1]; 
    uint8_t *raw_ptr = (uint8_t *)&tx;
    uint8_t checksum = 0;

    send_buf[0] = _start_byte;
    send_buf[1] = (uint8_t)data_size;

    for (size_t i = 0; i < data_size; i++) {
      send_buf[2 + i] = raw_ptr[i];
      checksum ^= raw_ptr[i];
    }
    send_buf[2 + data_size] = checksum;

    _serial->write(send_buf, sizeof(send_buf));
  }

  bool update(uint32_t send_interval_ms = 20)
  {
    bool received = read();

    if (millis() - _last_tx_time >= send_interval_ms) {
      _last_tx_time = millis();
      send();
    }

    return received;
  }

private:
  bool readPacket()
  {
    bool packet_received = false;
    while (_serial && _serial->available() > 0)
    {
      uint8_t b = _serial->read();
      switch (_rx_state)
      {
      case 0:
        if (b == _start_byte) _rx_state = 1;
        break;
      case 1:
        if (b == sizeof(rx_t)) {
          _expected_size = b;
          _rx_index = 0;
          _calc_checksum = 0;
          _rx_state = 2;
        } else {
          _rx_state = 0;
        }
        break;
      case 2:
        _rx_buffer[_rx_index++] = b;
        _calc_checksum ^= b;
        if (_rx_index >= _expected_size) _rx_state = 3;
        break;
      case 3:
        if (b == _calc_checksum) {
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