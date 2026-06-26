#include "packetManager.hpp"

PacketManager::~PacketManager()
{
    if (_data != nullptr)
    {
        delete[] _data;
        _data = nullptr;
    }
}

void PacketManager::setup(uint8_t start_header, int byte_size, uint8_t end_header)
{
    if (_data != nullptr)
    {
        delete[] _data;
        _data = nullptr;
    }

    _start_header = start_header;
    _byte_size = byte_size;
    _end_header = end_header;

    _data_capacity = _byte_size + 2;

    if (_byte_size <= 0 || _data_capacity > 255)
    {
        _data_capacity = 0;
        return;
    }

    _data = new uint8_t[_data_capacity];

    if (_data == nullptr)
    {
        _data_capacity = 0;
        _byte_size = 0;
    }

    for (int i = 0; i < _data_capacity; i++)
    {
        _data[i] = 0;
    }

    reset();
}

void PacketManager::reset()
{
    _next_index = 0;
}

void PacketManager::add(uint8_t byte)
{
    if (_data == nullptr || _data_capacity == 0)
        return;

    if (isComplete())
    {
        reset();
        _next_index = 0;
    }

    if (_next_index == 0)
    {
        if (byte == _start_header)
        {
            _data[_next_index] = byte;
            _next_index++;
        }
    }

    else if (_next_index >= 1 && _next_index <= _byte_size)
    {
        _data[_next_index] = byte;
        _next_index++;
    }

    else if (_next_index == _data_capacity - 1)
    {
        if (byte == _end_header)
        {
            _data[_next_index] = byte;
            _next_index++;
        }
        else
        {
            reset();
        }
    }
    else
    {
        reset();
    }
}

bool PacketManager::isComplete()
{
    if (_data_capacity > 0 && _next_index == _data_capacity)
        return true;

    return false;
}

uint8_t PacketManager::get(int index)
{
    if (_data == nullptr || index < 0 || index >= _data_capacity)
        return 0x00;

    return _data[index];
}