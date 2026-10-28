#include <Arduino.h>
#include <iostream>
#include <cstring>
#include <atomic>
#include "common/timer.hpp"
#include "device/button.hpp"
#include "ui.hpp"
#include "infra.hpp"
#include "record.hpp"

/* common */

// var
volatile uint32_t friends_count = 0; // 友達の(自分が受信した)メッセージの数
volatile uint32_t my_count = 0;      // 自分の(自分が送信した)メッセージの数

// signal
std::atomic<bool> j0_recv{false}; // j=0の未処理データがある
uint8_t j0_recv_cycle = 0;
uint32_t j0_recv_message_id = 0;
char j0_recv_text[infra::text_size_max];

std::atomic<bool> j1_recv{false}; // j=1の未処理応答がある
uint8_t j1_recv_cycle = 0;
uint32_t j1_recv_message_id = 0;
char j1_recv_text[infra::text_size_max];

std::atomic<bool> setup1_ok{false}; // setup1が完了した

// core0/core1が同じinfraオブジェクトを使うため、IR APIの同時呼び出しを防ぐ
std::atomic<bool> infrared_sending{false};
std::atomic<bool> infrared_reading{false};

// core1だけが使うACK送信状態
infra::data pending_ack_data{};
bool ack_pending = false;

// core1だけが使う未確認の送信データ
infra::data pending_outgoing_data{};
bool pending_outgoing_valid = false;
size_t pending_outgoing_text_size = 0;

constexpr uint32_t k_send_timeout_ms = 8000;
constexpr uint32_t k_data_retryInterval_ms = 2500;
constexpr uint32_t k_higherId_turn_delay_ms = 1200;

// func
/**
 * @brief 真の乱数生成関数
 *
 * @param min_val   乱数の最小値
 * @param max_val   乱数の最大値
 *
 * @return min_val から max_val までの整数
 */
int hardware_random(int min_val, int max_val)
{
    if (max_val < min_val)
    {
        return min_val;
    }

    uint64_t range =
        (uint64_t)((int64_t)max_val - (int64_t)min_val + 1);

    uint64_t threshold = (uint64_t)(-range) % range;
    uint64_t r;

    do
    {
        r = ((uint64_t)rp2040.hwrand32() << 32) | rp2040.hwrand32();
    } while (r < threshold);

    return (int)((int64_t)min_val + (int64_t)(r % range));
}

/**
 * @brief 周期カウント(T=256)同士を比較する関数
 * @attention 128を境にcycleの大小を比較 diffが128当たりの時は正確な大小比較ではなくなる
 *
 * @param[in]   cycle1  比較カウント
 * @param[in]   cycle2  比較カウント
 * @param[out]  diff    カウント同士の差(正のみ)
 *
 * @return 1 (cycle1>cycle2),または0 (cycle1==cycle2),または-1 (cycle1<cycle2)
 */
int compare_cycle(uint8_t cycle1, uint8_t cycle2, uint8_t &diff)
{
    const uint8_t forward = static_cast<uint8_t>(cycle1 - cycle2);

    if (forward == 0)
    {
        diff = 0;
        return 0;
    }

    if (forward < 128)
    {
        diff = forward;
        return 1;
    }

    if (forward > 128)
    {
        diff = static_cast<uint8_t>(256 - forward);
        return -1;
    }

    diff = 128;
    return cycle1 > cycle2 ? 1 : -1;
}

/* core0 */
// infra recv

// func
uint8_t get_id(uint8_t j) { return j & 0b01111111; }
bool get_j(uint8_t j) { return static_cast<bool>(j & 0b10000000); }

size_t packet_text_length(const char *text, size_t capacity)
{
    size_t length = 0;
    while (length < capacity && text[length] != '\0')
        length++;
    return length;
}

bool time_reached(uint32_t now, uint32_t target)
{
    return static_cast<int32_t>(now - target) >= 0;
}

bool try_read_infra(infra::data &received_data)
{
    // Core 0は読み取り中フラグを立ててから、Core 1の送信状態を確認する
    infrared_reading.store(true);
    if (infrared_sending.load())
    {
        infrared_reading.store(false);
        return false;
    }

    const bool received = infra::packet.read();
    if (received)
        received_data = infra::packet.rx;

    infrared_reading.store(false);
    return received;
}

void send_infra_data(const infra::data &outgoing_data)
{
    // Core 1は送信中フラグを立ててから、Core 0の読み取り完了を待つ
    infrared_sending.store(true);
    while (infrared_reading.load())
        delayMicroseconds(20);

    const infra::data previous_tx = infra::packet.tx;
    infra::packet.tx = outgoing_data;
    infra::packet.send();
    infra::packet.tx = previous_tx;

    infrared_sending.store(false);
}

void setup()
{
    while (!setup1_ok.load(std::memory_order_acquire)) // setup1が終わるまで待つ
        delay(1);
}

void loop()
{
    infra::data received_data{};
    const bool received = try_read_infra(received_data);

    if (!received)
        return;

    const bool is_ack = get_j(received_data.j);
    const uint8_t id = get_id(received_data.j);

    if (id != infra::friends_id)
        return;

    if (is_ack)
    {
        if (!j1_recv.load(std::memory_order_acquire))
        {
            j1_recv_cycle = received_data.cycle;
            j1_recv_message_id = received_data.message_id;
            memcpy(j1_recv_text, received_data.text, infra::text_size_max);
            j1_recv.store(true, std::memory_order_release);
        }
    }
    else
    {
        if (!j0_recv.load(std::memory_order_acquire))
        {
            j0_recv_cycle = received_data.cycle;
            j0_recv_message_id = received_data.message_id;
            memcpy(j0_recv_text, received_data.text, infra::text_size_max);
            j0_recv.store(true, std::memory_order_release);
        }
    }
}

/* core1 */
// infra send
// ui::display
// record

// func
void queue_ack(const infra::data &ack_data)
{
    pending_ack_data = ack_data;
    ack_pending = true;
}

void recv() // 受信データを解析し受信完了コードを送信する
{
    if (!j0_recv.load(std::memory_order_acquire))
        return;

    const uint8_t received_cycle = j0_recv_cycle;
    const uint32_t received_message_id = j0_recv_message_id;
    char received_text[infra::text_size_max];
    memcpy(received_text, j0_recv_text, infra::text_size_max);
    j0_recv.store(false, std::memory_order_release);

    const size_t received_text_size =
        packet_text_length(received_text, infra::text_size_max);

    infra::data ack_data{};
    ack_data.j = 0b10000000 | infra::my_id;
    ack_data.cycle = received_cycle;
    ack_data.message_id = received_message_id;

    const uint32_t expect_message_id = friends_count + 1;

    if (received_cycle != static_cast<uint8_t>(received_message_id))
        return;

    if (received_message_id == expect_message_id) // 受信正常成功
    {
        record::append(infra::friends_id, received_cycle, received_text, received_text_size);
        friends_count++;

        Serial.print("friend:");
        Serial.write(
            reinterpret_cast<const uint8_t *>(received_text),
            received_text_size);
        Serial.println();

        queue_ack(ack_data);
    }
    else if (received_message_id == friends_count) // 受信cycleが期待cycleより古い
    {
        // 同じmessage_idの再送なので本文は二重記録せず、ACKだけ再送する
        queue_ack(ack_data);
    }
    else // 受信cycleが期待cycleより新しい
    {
        // 期待する番号より先のデータは記録せず、最後に受理した番号を返す
        ack_data.message_id = friends_count;
        ack_data.cycle = static_cast<uint8_t>(friends_count);
        queue_ack(ack_data);
    }
}

void keep_recv()
{
    if (!ack_pending)
        return;

    // ACKは1回だけ送り、同じ本文が再送された場合はrecv()がACKを再度予約する
    ack_pending = false;
    ui::utf8_printwrapped("受信完了コードを送信中", 0, 42, 128);
    send_infra_data(pending_ack_data);
}

bool take_matching_ack(const infra::data &outgoing_data)
{
    if (!j1_recv.load(std::memory_order_acquire))
        return false;

    const uint8_t ack_cycle = j1_recv_cycle;
    const uint32_t ack_message_id = j1_recv_message_id;
    j1_recv.store(false, std::memory_order_release);

    return ack_cycle == outgoing_data.cycle &&
           ack_message_id == outgoing_data.message_id;
}

void print_log_history()
{
    size_t num = 0;
    uint8_t id, cycle, text_len;
    const char *text;

    while (record::get(num, id, cycle, text, text_len))
    {
        Serial.print(id == infra::my_id ? "you   :" : "friend:");
        Serial.write(
            reinterpret_cast<const uint8_t *>(text),
            text_len);
        Serial.println();
        ++num;
    }
}

void send(String c)
{
    const size_t text_size = c.length();

    if (text_size > infra::text_size_max) // サイズオーバーは送らない
        return;

    if (pending_outgoing_valid)
    {
        const bool same_text =
            text_size == pending_outgoing_text_size &&
            memcmp(
                pending_outgoing_data.text,
                c.c_str(),
                text_size) == 0;

        if (!same_text)
        {
            Serial.println("前回の未確認メッセージを同じ内容で再入力してください");
            return;
        }
    }
    else
    {
        memset(&pending_outgoing_data, 0, sizeof(pending_outgoing_data));
        if (text_size > 0)
            memcpy(pending_outgoing_data.text, c.c_str(), text_size);

        pending_outgoing_data.j = infra::my_id;
        pending_outgoing_data.message_id = my_count + 1;
        pending_outgoing_data.cycle =
            static_cast<uint8_t>(pending_outgoing_data.message_id);

        pending_outgoing_text_size = text_size;
        pending_outgoing_valid = true;
    }

    // 前回の応答を新しい送信のACKとして誤認しないようにする
    j1_recv.store(false, std::memory_order_release);

    const uint32_t send_started_at = millis();
    const uint32_t first_send_at =
        send_started_at + (infra::my_id > infra::friends_id ? k_higherId_turn_delay_ms : 0);
    uint32_t last_send_at = 0;
    bool has_sent = false;
    bool send_success = false;

    while (static_cast<uint32_t>(millis() - send_started_at) < k_send_timeout_ms)
    {
        ui::utf8_printwrapped("データを送信中", 0, 14, 128);

        if (j0_recv.load(std::memory_order_acquire))
            recv();

        if (take_matching_ack(pending_outgoing_data))
        {
            send_success = true;
            break;
        }

        keep_recv();

        const uint32_t now = millis();
        const bool first_send_due =
            !has_sent && time_reached(now, first_send_at);
        const bool retry_due =
            has_sent &&
            static_cast<uint32_t>(now - last_send_at) >= k_data_retryInterval_ms;

        if (first_send_due || retry_due)
        {
            send_infra_data(pending_outgoing_data);
            last_send_at = millis();
            has_sent = true;

            if (take_matching_ack(pending_outgoing_data))
            {
                send_success = true;
                break;
            }
        }

        ui::display.sendBuffer();
        ui::display.clearBuffer();
        delay(1);
    }

    if (send_success)
    {
        record::append(
            infra::my_id,
            pending_outgoing_data.cycle,
            pending_outgoing_data.text,
            pending_outgoing_text_size);

        my_count = pending_outgoing_data.message_id;
        pending_outgoing_valid = false;

        Serial.print("you   :");
        Serial.write(
            reinterpret_cast<const uint8_t *>(pending_outgoing_data.text),
            pending_outgoing_text_size);
        Serial.println();
    }

    ui::display.clearBuffer();
    ui::utf8_printwrapped((send_success ? "送信成功" : "送信失敗"), 0, 14, 128);
    ui::display.sendBuffer();
}

void setup1()
{
    Serial.begin(115200); // pc
    Serial.setTimeout(50);

    Wire1.setSDA(2); // GP2
    Wire1.setSCL(3); // GP3

    // ui::display
    ui::display.begin();
    ui::display.enableUTF8Print();
    ui::display.setFont(u8g2_font_b10_t_japanese1);
    ui::display.setFontPosBaseline();
    ui::display.clearBuffer();
    ui::display.sendBuffer();

    // infra
    infra::packet.begin(8, 26); // TX=GP8, RX=GP26

    // button
    front_btn.begin(4, INPUT_PULLUP);
    right_btn.begin(5, INPUT_PULLUP);
    left_btn.begin(6, INPUT_PULLUP);
    rear_btn.begin(7, INPUT_PULLUP);

    // id確定
    String a;
    char a_array[31];

    uint32_t last_time = millis();
    do
    {
        infra::my_id = hardware_random(0, 127);
        infra::packet.tx.j = (infra::my_id) | 0b10000000; // 8bit目を1にしてj確定
        infra::packet.tx.cycle = 0;
        infra::packet.tx.message_id = 0;
        memset(infra::packet.tx.text, 0, infra::text_size_max);

        while (!infra::packet.update())
        {
            ui::display.clearBuffer();

            a = "自分のID:" + String(infra::my_id);
            strncpy(a_array, a.c_str(), 31);
            ui::utf8_printwrapped(a_array, 0, 14, 128);
            uint32_t pass_time = millis() - last_time;
            a = "友達探索中... " + String(static_cast<int>(pass_time / 1000)) + "秒";
            strncpy(a_array, a.c_str(), 31);
            ui::utf8_printwrapped(a_array, 0, 28, 128);

            ui::display.sendBuffer();
        }
        infra::friends_id = (infra::packet.rx.j) & 0b01111111; // 8bit目を0にしてfriends_id確定
    } while (infra::my_id == infra::friends_id);

    timer keep;
    keep.reset();
    while (keep.msTime() < 5000)
    {
        infra::packet.update();

        ui::display.clearBuffer();

        a = "自分のID:" + String(infra::my_id);
        strncpy(a_array, a.c_str(), 31);
        ui::utf8_printwrapped(a_array, 0, 14, 128);
        a = "友達のID:" + String(infra::friends_id);
        strncpy(a_array, a.c_str(), 31);
        ui::utf8_printwrapped(a_array, 0, 28, 128);
        a = "通信安定化... " + String(static_cast<int>(keep.msTime() / 1000)) + "秒";
        strncpy(a_array, a.c_str(), 31);
        ui::utf8_printwrapped(a_array, 0, 42, 128);

        ui::display.sendBuffer();
    }

    ui::display.clearBuffer();

    a = "自分のID:" + String(infra::my_id);
    strncpy(a_array, a.c_str(), 31);
    ui::utf8_printwrapped(a_array, 0, 14, 128);
    a = "友達のID:" + String(infra::friends_id);
    strncpy(a_array, a.c_str(), 31);
    ui::utf8_printwrapped(a_array, 0, 28, 128);

    ui::display.sendBuffer();

    infra::packet.rx.cycle = 0;
    infra::packet.rx.message_id = 0;
    infra::packet.rx.j = 0;
    infra::packet.tx.cycle = 0;
    infra::packet.tx.message_id = 0;
    infra::packet.tx.j = 0;
    delay(1000);

    setup1_ok.store(true, std::memory_order_release);
}

void loop1()
{
    bool send_ready = false;
    String c;
    if (Serial.available() > 0)
    {
        c = Serial.readStringUntil('\n');
        c.trim();

        if (c == "show")
        {
            print_log_history();
        }
        else if (c.length() > 0 && c.length() <= infra::text_size_max)
        {
            send_ready = true;
        }
        else if (c.length() > infra::text_size_max)
        {
            Serial.println("文章を短くしてください");
        }
    }

    if (send_ready)
    {
        send(c);
    }
    else if (j0_recv.load(std::memory_order_acquire))
    {
        recv();
    }

    keep_recv();

    ui::display.sendBuffer();
    ui::display.clearBuffer();
}