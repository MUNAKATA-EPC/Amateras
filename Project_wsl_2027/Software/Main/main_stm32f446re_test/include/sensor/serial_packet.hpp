#pragma once
#include <Arduino.h>

template <class tx_t, class rx_t>
class serial_packet
{
private:
  HardwareSerial *_serial = nullptr;
  uint32_t _last_tx_time = 0;
  uint32_t _tx_interval_ms;
  const uint8_t _start_byte = 0xAA;

public:
  tx_t tx;
  rx_t rx;

  serial_packet(uint32_t interval_ms = 20)
  {
    _tx_interval_ms = interval_ms;
    tx = tx_t();
    rx = rx_t();
  }

  void begin(HardwareSerial &serial_obj)
  {
    _serial = &serial_obj;
  }

  bool update()
  {
    if (!_serial)
      return false;
    if (millis() - _last_tx_time >= _tx_interval_ms)
    {
      _last_tx_time = millis();
      sendPacket();
    }
    return readPacket();
  }

private:
  void sendPacket()
  {
    uint8_t *raw_ptr = (uint8_t *)&tx;
    size_t size = sizeof(tx_t);
    uint8_t checksum = 0;

    _serial->write(_start_byte);
    _serial->write(size);

    for (size_t i = 0; i < size; i++)
    {
      _serial->write(raw_ptr[i]);
      checksum ^= raw_ptr[i];
    }
    _serial->write(checksum);
  }

  bool readPacket()
  {
    static int state = 0;
    static size_t rx_index = 0;
    static uint8_t expected_size = 0;
    static uint8_t calc_checksum = 0;
    static uint8_t rx_buffer[sizeof(rx_t) > 0 ? sizeof(rx_t) : 1];
    bool packet_received = false;

    while (_serial->available() > 0)
    {
      uint8_t b = _serial->read();
      switch (state)
      {
      case 0:
        if (b == _start_byte)
          state = 1;
        break;
      case 1:
        if (b == sizeof(rx_t))
        {
          expected_size = b;
          rx_index = 0;
          calc_checksum = 0;
          state = 2;
        }
        else
        {
          state = 0;
        }
        break;
      case 2:
        rx_buffer[rx_index++] = b;
        calc_checksum ^= b;
        if (rx_index >= expected_size)
          state = 3;
        break;
      case 3:
        if (b == calc_checksum)
        {
          memcpy(&rx, rx_buffer, sizeof(rx_t));
          packet_received = true;
        }
        state = 0;
        break;
      }
    }
    return packet_received;
  }
};