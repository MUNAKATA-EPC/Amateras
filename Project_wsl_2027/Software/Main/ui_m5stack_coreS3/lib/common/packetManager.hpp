#pragma once

#include "Arduino.h"

class PacketManager
{
private:
    uint8_t *_data = nullptr;
    int _data_capacity = 0;
    int _byte_size = 0;
    int _next_index = 0;

    uint8_t _start_header = 0x55;
    uint8_t _end_header = 0xAA;

public:
    PacketManager() = default;
    ~PacketManager();

    PacketManager(const PacketManager &) = delete;
    PacketManager &operator=(const PacketManager &) = delete;

    void setup(uint8_t start_header, int byte_size, uint8_t end_header);

    void reset();
    void add(uint8_t byte);
    bool isComplete();
    uint8_t get(int index);
};