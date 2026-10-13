#pragma once

#include <stdio.h>
#include <string.h>

#include <Wire.h>
#include <U8g2lib.h>

// textrecordの容量は最大レコード（方向 + 長さ + text）以上にする（十分に確保せよ）
const char textrecord_recv = 127;  // recv記号
const char textrecord_send = 126;  // send記号
char textrecord[2048];             // 記録形式は (textrecord_recv/send) (本文の長さ) (text)
size_t textrecord_targetindex = 0; // 次に記録するインデックス

// サイズ
const size_t textrecord_size = sizeof(textrecord);

// textrecordに記録する
void appendTextrecord(uint8_t direction, const char *data, uint8_t len)
{
    if (len > text_size - 1)
        return;

    const size_t needed = 2 + len; // 方向 + 長さ + 本文

    while (textrecord_targetindex + needed > textrecord_size)
    {
        // 先頭のレコードを削除する
        const uint8_t old_len = static_cast<uint8_t>(textrecord[1]);
        const size_t old_size = 2 + old_len;

        // 念のため、記録データの状態が不正なら空にする
        if (old_size > textrecord_targetindex)
        {
            textrecord_targetindex = 0;
            break;
        }

        memmove(textrecord,
                textrecord + old_size,
                textrecord_targetindex - old_size);
        textrecord_targetindex -= old_size;
    }

    textrecord[textrecord_targetindex++] = static_cast<char>(direction);
    textrecord[textrecord_targetindex++] = static_cast<char>(len);

    if (len > 0)
    {
        memcpy(textrecord + textrecord_targetindex, data, len);
        textrecord_targetindex += len;
    }
}

// textrecordのヘッダーを読み取る
bool readRecordHeader(const char *data, size_t used, size_t idx,
                      uint8_t &direction, uint8_t &len)
{
    if (idx > used || used - idx < 2)
        return false;

    direction = static_cast<uint8_t>(data[idx]);
    len = static_cast<uint8_t>(data[idx + 1]);

    if (direction != static_cast<uint8_t>(textrecord_recv) &&
        direction != static_cast<uint8_t>(textrecord_send))
        return false;

    if (len > text_size - 1 || len > used - idx - 2)
        return false;

    return true;
}