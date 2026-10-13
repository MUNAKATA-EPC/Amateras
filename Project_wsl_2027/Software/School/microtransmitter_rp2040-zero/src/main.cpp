#include <Arduino.h>
#include "common.hpp"
#include "core0.hpp"
#include "core1.hpp"

void setup()
{
    Serial.begin(115200); // pc

    Wire1.setSDA(2); // GP2
    Wire1.setSCL(3); // GP3

    // display
    display.begin();
    display.enableUTF8Print();
    display.setFont(u8g2_font_b10_t_japanese1);
    display.setFontPosBaseline();
    display.clearBuffer();
    display.sendBuffer();

    // infra
    infra.begin(8, 26); // TX=GP8, RX=GP26
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
            utf8_printwrapped(infra.rx.text, 0, 14, 128);
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
            utf8_printwrapped(infra.tx.text, 0, 14, 128);
            display.sendBuffer();
        }
        else
        {
            Serial.println("trim your text to less than " +
                           String(text_size - 1) + " bytes");
        }
    }
}

/*
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <atomic>

// packet の実サイズは count 1 byte + text 51 bytes = 52 bytes。
#define INFRA_PACKET_MAX_PAYLOAD (52 + 2)
#define NO_LED_SEND_FEEDBACK_CODE
#include "common/infra_packet.hpp"

#include <Wire.h>
#include <U8g2lib.h>

// Core 1だけがOLEDを操作する。
U8G2_SSD1306_128X64_NONAME_F_2ND_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE);

constexpr uint8_t kTextCapacity = 50; // UTF-8では文字数でなくバイト数
constexpr uint8_t kTextBufferSize = kTextCapacity + 1;
constexpr uint8_t kRecordRecv = 127;
constexpr uint8_t kRecordSend = 126;
constexpr size_t kTextRecordSize = 2048;
constexpr size_t kSerialLineBufferSize = 160;
constexpr uint32_t kSerialIdleFlushMs = 500;

struct packet
{
    uint8_t count = 0;
    char text[kTextBufferSize] = {};
} __attribute__((packed));

infra_packet<packet, packet> infra;

struct DisplayEvent
{
    uint8_t direction = 0;
    uint8_t len = 0;
    char text[kTextBufferSize] = {};
};

struct TxRequest
{
    uint8_t count = 0;
    uint8_t len = 0;
    char text[kTextBufferSize] = {};
};

// 単一producer / 単一consumerの固定長キュー。
// DisplayEvent: Core 0 -> Core 1、TxRequest: Core 1 -> Core 0。
template <class T, uint8_t Capacity>
class SpscQueue
{
private:
    static_assert(Capacity >= 2, "Queue capacity must be at least 2");
    T _items[Capacity] = {};
    std::atomic<uint8_t> _head{0};
    std::atomic<uint8_t> _tail{0};

public:
    // 1枠を空/満杯の区別に使うため、Capacity=16なら15件保持できる。
    bool push(const T &item)
    {
        const uint8_t head = _head.load(std::memory_order_relaxed);
        const uint8_t next = static_cast<uint8_t>((head + 1) % Capacity);
        if (next == _tail.load(std::memory_order_acquire))
            return false;

        _items[head] = item;
        _head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T &item)
    {
        const uint8_t tail = _tail.load(std::memory_order_relaxed);
        if (tail == _head.load(std::memory_order_acquire))
            return false;

        item = _items[tail];
        const uint8_t next = static_cast<uint8_t>((tail + 1) % Capacity);
        _tail.store(next, std::memory_order_release);
        return true;
    }
};

SpscQueue<DisplayEvent, 16> displayQueue;
SpscQueue<TxRequest, 8> txQueue;

// Serial.begin()はCore 0、USB Serialの読み書きはCore 1に集約する。
volatile bool serialReady = false;

// Core 1だけが所有する表示履歴。
char textrecord[kTextRecordSize] = {};
size_t textrecord_targetindex = 0;
DisplayEvent latestDisplayEvent;
bool hasDisplayEvent = false;
bool displayDirty = false;
uint8_t nextTxCount = 0;

// Core 1だけが所有するSerial行バッファ。
char serialLine[kSerialLineBufferSize] = {};
size_t serialLineLength = 0;
bool serialLineOverflow = false;
bool previousSerialCharWasCR = false;
uint32_t serialLastByteAt = 0;

// Core 0だけが所有する送信待ち状態。
bool pendingTx = false;
uint8_t txRepeatsLeft = 0;
uint32_t txNotBefore = 0;

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

void printWrappedUTF8(
    const char *text,
    uint16_t x,
    uint16_t baselineY,
    uint16_t maxWidth,
    uint8_t asciiGapPx = 1)
{
    const uint16_t screenWidth = display.getDisplayWidth();
    if (x >= screenWidth)
        return;
    if (maxWidth > screenWidth - x)
        maxWidth = screenWidth - x;

    const uint16_t rightEdge = x + maxWidth;
    const uint16_t linePitch = display.getMaxCharHeight() + 2;
    uint16_t cursorX = x;
    uint16_t cursorY = baselineY;
    const uint8_t *p = reinterpret_cast<const uint8_t *>(text);

    while (*p != '\0')
    {
        if (*p == '\r')
        {
            ++p;
            continue;
        }
        if (*p == '\n')
        {
            cursorX = x;
            cursorY += linePitch;
            ++p;
            continue;
        }

        const uint8_t bytes = utf8CharBytes(p);
        char glyph[5];
        for (uint8_t i = 0; i < bytes; ++i)
            glyph[i] = static_cast<char>(p[i]);
        glyph[bytes] = '\0';

        const uint16_t glyphWidth = display.getUTF8Width(glyph);
        const bool isAsciiLetterOrDigit =
            bytes == 1 &&
            ((glyph[0] >= 'A' && glyph[0] <= 'Z') ||
             (glyph[0] >= 'a' && glyph[0] <= 'z') ||
             (glyph[0] >= '0' && glyph[0] <= '9'));
        const uint8_t gap = isAsciiLetterOrDigit ? asciiGapPx : 0;

        if (cursorX > x && cursorX + glyphWidth > rightEdge)
        {
            cursorX = x;
            cursorY += linePitch;
        }

        display.setCursor(cursorX, cursorY);
        display.print(glyph);
        cursorX += glyphWidth + gap;
        p += bytes;
    }
}

void appendTextrecord(uint8_t direction, const char *data, uint8_t len)
{
    if (len > kTextCapacity)
        return;

    const size_t needed = 2 + len;
    while (textrecord_targetindex + needed > kTextRecordSize)
    {
        if (textrecord_targetindex < 2)
        {
            textrecord_targetindex = 0;
            break;
        }

        const uint8_t oldLen = static_cast<uint8_t>(textrecord[1]);
        const size_t oldSize = 2 + oldLen;
        if (oldSize > textrecord_targetindex)
        {
            textrecord_targetindex = 0;
            break;
        }

        memmove(textrecord,
                textrecord + oldSize,
                textrecord_targetindex - oldSize);
        textrecord_targetindex -= oldSize;
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
    if ((direction != kRecordRecv && direction != kRecordSend) ||
        len > kTextCapacity || len > used - idx - 2)
        return false;
    return true;
}

void showTextrecord()
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

        Serial.print(direction == kRecordRecv ? "friend:" : "you   :");
        Serial.write(reinterpret_cast<const uint8_t *>(textrecord + i + 2), len);
        Serial.println();
        i += 2 + len;
    }
}

void setLatestDisplayEvent(uint8_t direction, const char *data, uint8_t len)
{
    if (len > kTextCapacity)
        return;

    latestDisplayEvent = DisplayEvent{};
    latestDisplayEvent.direction = direction;
    latestDisplayEvent.len = len;
    if (len > 0)
        memcpy(latestDisplayEvent.text, data, len);
    latestDisplayEvent.text[len] = '\0';
    hasDisplayEvent = true;
    displayDirty = true;
}

void drawLatestEvent()
{
    if (!hasDisplayEvent)
        return;

    display.clearBuffer();
    display.setCursor(0, 13);
    display.print(latestDisplayEvent.direction == kRecordRecv
                      ? "friend:"
                      : "you   :");
    printWrappedUTF8(latestDisplayEvent.text, 0, 28, 128);
    display.sendBuffer();
}

void finishSerialLine()
{
    if (serialLineOverflow)
    {
        Serial.println("input line too long; trim it to 50 bytes or fewer");
    }
    else
    {
        serialLine[serialLineLength] = '\0';

        size_t begin = 0;
        size_t end = strlen(serialLine);
        while (begin < end && isspace(static_cast<unsigned char>(serialLine[begin])))
            ++begin;
        while (end > begin && isspace(static_cast<unsigned char>(serialLine[end - 1])))
            --end;
        const size_t len = end - begin;
        if (begin > 0)
            memmove(serialLine, serialLine + begin, len);
        serialLine[len] = '\0';

        if (len > 0)
        {
            if (strcmp(serialLine, "show") == 0)
            {
                showTextrecord();
            }
            else if (len > kTextCapacity)
            {
                Serial.println("trim your text to 50 bytes or fewer");
            }
            else
            {
                TxRequest request{};
                request.count = (nextTxCount > 200)
                                    ? 0
                                    : static_cast<uint8_t>(nextTxCount + 1);
                request.len = static_cast<uint8_t>(len);
                memcpy(request.text, serialLine, len);
                request.text[len] = '\0';

                if (txQueue.push(request))
                {
                    nextTxCount = request.count;
                    appendTextrecord(kRecordSend, serialLine, request.len);
                    setLatestDisplayEvent(kRecordSend, serialLine, request.len);
                    Serial.print("[TX queued] bytes=");
                    Serial.println(request.len);
                }
                else
                {
                    Serial.println("[TX queue full] wait and try again");
                }
            }
        }
    }

    serialLineLength = 0;
    serialLineOverflow = false;
}

void pollSerialInput()
{
    while (Serial.available() > 0)
    {
        const char ch = static_cast<char>(Serial.read());
        serialLastByteAt = millis();
        if (ch == '\r' || ch == '\n')
        {
            const bool secondCrLfChar = (ch == '\n' && previousSerialCharWasCR);
            previousSerialCharWasCR = (ch == '\r');
            if (!secondCrLfChar)
                finishSerialLine();
            continue;
        }

        previousSerialCharWasCR = false;
        if (!serialLineOverflow)
        {
            if (serialLineLength + 1 < kSerialLineBufferSize)
                serialLine[serialLineLength++] = ch;
            else
                serialLineOverflow = true;
        }
    }

    // 改行を送らないシリアルモニターでも、入力が止まったら1行として扱う。
    if ((serialLineLength > 0 || serialLineOverflow) &&
        millis() - serialLastByteAt >= kSerialIdleFlushMs)
    {
        finishSerialLine();
        previousSerialCharWasCR = false;
    }
}

void handleReceivedPacket()
{
    static bool havePrevious = false;
    static uint8_t previousCount = 0;
    static uint8_t previousLen = 0;
    static char previousText[kTextBufferSize] = {};

    const uint8_t count = infra.rx.count;
    size_t len = 0;
    while (len < kTextCapacity && infra.rx.text[len] != '\0')
        ++len;

    const bool isNew = !havePrevious ||
                       count != previousCount ||
                       len != previousLen ||
                       memcmp(infra.rx.text, previousText, len) != 0;

    previousCount = count;
    previousLen = static_cast<uint8_t>(len);
    memcpy(previousText, infra.rx.text, len);
    previousText[len] = '\0';
    havePrevious = true;

    // 起動時に相手が繰り返す空の初期パケットは表示しない。
    if (!isNew || len == 0)
        return;

    DisplayEvent event{};
    event.direction = kRecordRecv;
    event.len = static_cast<uint8_t>(len);
    memcpy(event.text, infra.rx.text, len);
    event.text[len] = '\0';
    if (!displayQueue.push(event))
        return; // OLED側が遅れても赤外線受信処理を止めない
}

bool timeReached(uint32_t now, uint32_t target)
{
    return static_cast<int32_t>(now - target) >= 0;
}

void loadNextTxRequest()
{
    if (pendingTx)
        return;

    TxRequest request;
    if (!txQueue.pop(request))
        return;

    infra.tx.count = request.count;
    memset(infra.tx.text, 0, sizeof(infra.tx.text));
    memcpy(infra.tx.text, request.text, request.len);

    pendingTx = true;
    txRepeatsLeft = 2; // 2回送って受信機会を増やす。受信側はcountで重複を除く。
    txNotBefore = millis() + 50 + (micros() % 800); // 同時送信の衝突を減らす
}

// Core 0: Infraだけを所有する。update()の自動定期送信は使わず、
// USB Serial/OLEDを止めずに済むよう、要求がある時だけsend()する。
void setup()
{
    Serial.begin(115200); // Arduino-PicoのUSB CDCではbaud値は実通信速度に使われない
    infra.begin(8, 26);    // TX=GP8, RX=GP26
    serialReady = true;
}

void loop()
{
    // 受信は常にポーリングする。Infraとinfra.rxはCore 0だけから触る。
    if (infra.read())
        handleReceivedPacket();

    loadNextTxRequest();

    if (pendingTx &&
        timeReached(millis(), txNotBefore) &&
        IrReceiver.isIdle())
    {
        infra.send(); // 送信中Core 0はブロックするが、Serial/OLEDのCore 1は動き続ける。
        --txRepeatsLeft;
        if (txRepeatsLeft == 0)
        {
            pendingTx = false;
        }
        else
        {
            txNotBefore = millis() + 80 + (micros() % 250);
        }
    }
}

// Core 1: OLED、USB Serial、recordtextを所有する。
void setup1()
{
    Wire1.setSDA(2); // GP2
    Wire1.setSCL(3); // GP3
    display.begin();
    display.enableUTF8Print();
    display.setFont(u8g2_font_b10_t_japanese1);
    display.setFontPosBaseline();
    display.clearBuffer();
    display.setCursor(0, 14);
    display.print("RP2040 Core 1");
    display.setCursor(0, 30);
    display.print("OLED ready");
    display.setCursor(0, 46);
    display.print("waiting for message");
    display.sendBuffer();
}

void loop1()
{
    // Serial処理はCore 1で続くため、赤外線send()がCore 0を占有しても入力を逃さない。
    if (serialReady)
        pollSerialInput();

    DisplayEvent event;
    while (displayQueue.pop(event))
    {
        appendTextrecord(event.direction, event.text, event.len);
        setLatestDisplayEvent(event.direction, event.text, event.len);
    }

    if (displayDirty)
    {
        displayDirty = false;
        drawLatestEvent();
    }
}
*/