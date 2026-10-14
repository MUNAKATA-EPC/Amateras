#pragma once

#define INFRA_PACKET_MAX_PAYLOAD (53 + 2) // 送るデータのバイト数に2を足したものを代入
#define NO_LED_SEND_FEEDBACK_CODE         // ledデバックを消す
#include "common/infra_packet.hpp"

uint8_t my_id = 0b00000000;      // 7bitまでをidとして使用
uint8_t friends_id = 0b00000000; // 7bitまでをidとして使用

/**
 * 物理ノイズ（ADC）を利用した簡易真の乱数生成関数
 * @param pin 接続したアナログピンの番号（例: A0 / GPIO 26）
 * @param min_val 乱数の最小値
 * @param max_val 乱数の最大値
 * @return min_val から max_val までの整数
 */
/**
 * 物理ノイズ（ADC）から十分なエントロピーを取得し、指定された範囲の乱数を返す
 * @param pin 接続したアナログピンの番号（例: A0 / GPIO 26）
 * @param min_val 乱数の最小値
 * @param max_val 乱数の最大値
 * @return min_val から max_val までの整数
 */
int hardware_randam(int8_t pin, int min_val, int max_val)
{
    (void)pin; // この方法ではADCピンを使わない

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

// infra
struct packet // 50文字まで送る
{
    uint8_t j = 0; // 7bitまではid 8bit目は送信/受信完了判別コード
    uint8_t count = 0;
    char text[50 + 1];
} __attribute__((packed));

infra_packet<packet, packet> infra;

// サイズ
const size_t text_size = sizeof(infra.tx.text);