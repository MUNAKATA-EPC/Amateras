#include <Arduino.h>

#include <stdio.h>
#include <string.h>

#define INFRA_PACKET_MAX_PAYLOAD (52 + 2) // 送るデータのバイト数に2を足したものを代入
#define NO_LED_SEND_FEEDBACK_CODE         // ledデバックを消す
#include "common/infra_packet.hpp"

#include <Wire.h>
#include <U8g2lib.h>

// display
// 128x64 SSD1306、ハードウェアI2C、フルバッファ、リセット線なし
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE);

// UTF-8文字が何バイトかを調べる（日本語は通常3バイト）
uint8_t utf8CharBytes(const uint8_t *p)
{
  if ((*p & 0x80) == 0x00)
    return 1;
  if ((*p & 0xE0) == 0xC0)
    return 2;
  if ((*p & 0xF0) == 0xE0)
    return 3;
  if ((*p & 0xF8) == 0xF0)
    return 4;
  return 1;
}

// x, baselineY を開始位置に、maxWidth の範囲で自動改行して描画
void printWrappedUTF8(
  const char* text,
  uint16_t x,
  uint16_t baselineY,
  uint16_t maxWidth,
  uint8_t asciiGapPx = 1
) {
  const uint16_t screenWidth = display.getDisplayWidth();

  if (x >= screenWidth) return;
  if (maxWidth > screenWidth - x) {
    maxWidth = screenWidth - x;
  }

  const uint16_t rightEdge = x + maxWidth;
  const uint16_t linePitch = display.getMaxCharHeight() + 2;

  uint16_t cursorX = x;
  uint16_t cursorY = baselineY;
  const uint8_t* p = (const uint8_t*)text;

  while (*p != '\0') {
    if (*p == '\r') {
      p++;
      continue;
    }
    if (*p == '\n') {
      cursorX = x;
      cursorY += linePitch;
      p++;
      continue;
    }

    uint8_t bytes = utf8CharBytes(p);
    char glyph[5];

    for (uint8_t i = 0; i < bytes; i++) {
      glyph[i] = (char)p[i];
    }
    glyph[bytes] = '\0';

    uint16_t glyphWidth = display.getUTF8Width(glyph);

    // 英字・数字の後ろに指定ピクセル分の間隔を足す
    bool isAsciiLetterOrDigit =
      bytes == 1 &&
      ((glyph[0] >= 'A' && glyph[0] <= 'Z') ||
       (glyph[0] >= 'a' && glyph[0] <= 'z') ||
       (glyph[0] >= '0' && glyph[0] <= '9'));

    uint8_t gap = isAsciiLetterOrDigit ? asciiGapPx : 0;

    // 次の文字と間隔が右端を超えるなら改行
    if (cursorX > x && cursorX + glyphWidth > rightEdge) {
      cursorX = x;
      cursorY += linePitch;
    }

    display.setCursor(cursorX, cursorY);
    display.print(glyph);

    cursorX += glyphWidth + gap;
    p += bytes;
  }
}

// infra
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

void setup()
{
  Serial.begin(9600);

  // display
  display.begin();
  display.enableUTF8Print();                  // print() にUTF-8文字列を渡せるようにする
  display.setFont(u8g2_font_b10_t_japanese1); // 日本語フォント
  display.setFontPosBaseline();
  display.clearBuffer(); // ブッファ初期化
  display.sendBuffer();  // バッファの内容をOLEDへ送る
  // display.drawFrame(0, 0, 128, 64); // 枠を描画

  // infra
  infra.begin(9, 10);
}

void loop()
{
  if (infra.update())
  {
    // rx
    static uint8_t last_count = 0;
    uint8_t cur_count = infra.rx.count;

    if (cur_count != last_count)
    {
      size_t rx_len = 0;
      while (rx_len < text_size && infra.rx.text[rx_len] != '\0')
      {
        rx_len++;
      }

      appendTextrecord(static_cast<uint8_t>(textrecord_recv),
                       infra.rx.text,
                       static_cast<uint8_t>(rx_len));

      // displayに描画
      display.clearBuffer();
      printWrappedUTF8(infra.rx.text, 0, 14, 128);
      display.sendBuffer();
    }

    last_count = cur_count;
  }

  // tx
  if (Serial.available())
  {
    String c = Serial.readStringUntil('\n');
    c.trim();

    //// debug
    if (c == "show")
    {
      size_t i = 0;

      while (i < textrecord_targetindex)
      {
        uint8_t direction;
        uint8_t len;

        if (!readRecordHeader(textrecord, textrecord_targetindex,
                              i, direction, len))
        {
          Serial.println("[record error]");
          break;
        }

        Serial.print(direction == static_cast<uint8_t>(textrecord_recv)
                         ? "friend:"
                         : "you   :");

        Serial.write(
            reinterpret_cast<const uint8_t *>(textrecord + i + 2), len);
        Serial.println();

        i += 2 + len;
      }

      return; // "show" 自体は送信しない
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
      appendTextrecord(static_cast<uint8_t>(textrecord_send),
                       c.c_str(),
                       static_cast<uint8_t>(c.length()));

      // displayに描画
      display.clearBuffer();
      printWrappedUTF8(infra.tx.text, 0, 14, 128);
      display.sendBuffer();
    }
    else
    {
      Serial.println("trim your text to less than " +
                     String(text_size - 1) + " bytes");
    }
  }
}