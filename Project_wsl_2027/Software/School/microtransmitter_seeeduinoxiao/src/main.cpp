#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#define INFRA_PACKET_MAX_PAYLOAD (52 + 2) // 送るデータのバイト数に2を足したものを代入
#define NO_LED_SEND_FEEDBACK_CODE         // ledデバックを消す
#include "common/infra_packet.hpp"

struct packet // 50文字まで送る
{
  uint8_t count = 0;
  char text[50 + 1];
} __attribute__((packed));

infra_packet<packet, packet> infra;

// textrecordの容量は最大レコード（方向 + 長さ + text）以上にする（十分に確保せよ）
const char textrecord_recv = 127;  // recv記号
const char textrecord_send = 126;  // send記号
char textrecord[2048];             // 記録形式は (textrecord_recv/send) (本文の長さ) (text)
size_t textrecord_targetindex = 0; // 次に記録するインデックス

// サイズ
const size_t text_size = sizeof(infra.tx.text);
const size_t textrecord_size = sizeof(textrecord);

void appendRecord(uint8_t direction, const char *data, uint8_t len)
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

void setup()
{
  Serial.begin(9600);

  infra.begin(9, 10);
}

void loop()
{
  if (infra.update())
  {
    // rx
    static uint8_t last_count = 0;
    static bool has_last_count = false;
    uint8_t cur_count = infra.rx.count;

    if (!has_last_count || cur_count != last_count)
    {
      size_t rx_len = 0;
      while (rx_len < text_size && infra.rx.text[rx_len] != '\0')
      {
        rx_len++;
      }

      appendRecord(static_cast<uint8_t>(textrecord_recv),
                   infra.rx.text,
                   static_cast<uint8_t>(rx_len));
    }

    last_count = cur_count;
    has_last_count = true;
  }

  // tx
  if (Serial.available())
  {
    String c = Serial.readStringUntil('\n');
    c.trim();

    ////
    if (c == "show")
    {
      for (size_t i = 0; i < textrecord_targetindex; i++)
      {
        if (textrecord[i] == textrecord_recv)
          Serial.print("friend:");
        else if (textrecord[i] == textrecord_send)
          Serial.print("you   :");
        else
          Serial.print(textrecord[i]);
        if (textrecord[i + 1] == textrecord_recv || textrecord[i + 1] == textrecord_send)
          Serial.println("");
      }
    }
    ////

    if (c.length() < text_size)
    {
      // countを上昇させる
      uint8_t cur_count = infra.tx.count;
      if (cur_count > 200)
        cur_count = 0;
      else
        cur_count++;
      infra.tx.count = cur_count;

      // text代入
      strncpy(infra.tx.text, c.c_str(), text_size); // 50バイト以下の本文と余分な部分を'\0'で埋めてコピーする

      // textrecordに記録
      appendRecord(static_cast<uint8_t>(textrecord_send),
                   c.c_str(),
                   static_cast<uint8_t>(c.length()));
    }
    else
    {
      Serial.println("trim your text to less than " +
                     String(text_size - 1) + " bytes");
    }
  }
}